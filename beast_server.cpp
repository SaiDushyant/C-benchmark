#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "connection_pool.h"

#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include "config.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(
        tcp::socket socket,
        std::shared_ptr<ConnectionPool> pool)
        : socket_(std::move(socket)),
          pool_(std::move(pool))
    {
    }

    void run()
    {
        read();
    }

private:
    tcp::socket socket_;

    std::shared_ptr<ConnectionPool> pool_;

    beast::flat_buffer buffer_;

    http::request<http::string_body> req_;

    void read()
    {
        req_ = {};

        http::async_read(
            socket_,
            buffer_,
            req_,
            beast::bind_front_handler(
                &Session::on_read,
                shared_from_this()));
    }

    void on_read(
        beast::error_code ec,
        std::size_t)
    {
        if (ec == http::error::end_of_stream)
        {
            socket_.shutdown(tcp::socket::shutdown_send, ec);
            return;
        }

        if (ec)
            return;

        std::string body;

        try
        {
            // Acquire a PostgreSQL connection from the pool.
            auto conn = pool_->acquire();

            pqxx::work tx(*conn);

            auto row = tx.exec(
                             "SELECT id, name, email "
                             "FROM users "
                             "WHERE id = 1")
                           .one_row();

            body =
                std::string("{") +
                "\"id\":" + std::string(row[0].c_str()) +
                ",\"name\":\"" + std::string(row[1].c_str()) +
                "\",\"email\":\"" + std::string(row[2].c_str()) +
                "\"}";
        }
        catch (const std::exception &e)
        {
            body =
                std::string("{\"error\":\"") +
                e.what() +
                "\"}";
        }

        auto res =
            std::make_shared<
                http::response<http::string_body>>(
                http::status::ok,
                req_.version());

        res->set(http::field::server, "benchmark");
        res->set(http::field::content_type, "application/json");
        res->keep_alive(req_.keep_alive());

        res->body() = std::move(body);

        res->prepare_payload();

        http::async_write(
            socket_,
            *res,
            [self = shared_from_this(), res](beast::error_code ec, std::size_t)
            {
                if (ec)
                    return;

                if (res->keep_alive())
                {
                    self->read();
                }
                else
                {
                    beast::error_code ignored;

                    self->socket_.shutdown(
                        tcp::socket::shutdown_send,
                        ignored);
                }
            });
    }
};

class Listener
{
public:
    Listener(
        asio::io_context &ioc,
        tcp::endpoint endpoint,
        std::shared_ptr<ConnectionPool> pool)
        : acceptor_(ioc),
          pool_(std::move(pool))
    {
        beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec)
            throw beast::system_error(ec);

        acceptor_.set_option(
            asio::socket_base::reuse_address(true),
            ec);
        if (ec)
            throw beast::system_error(ec);

        acceptor_.bind(endpoint, ec);
        if (ec)
            throw beast::system_error(ec);

        acceptor_.listen(
            asio::socket_base::max_listen_connections,
            ec);
        if (ec)
            throw beast::system_error(ec);
    }

    void run()
    {
        accept();
    }

private:
    tcp::acceptor acceptor_;

    std::shared_ptr<ConnectionPool> pool_;

    void accept()
    {
        acceptor_.async_accept(
            beast::bind_front_handler(
                &Listener::on_accept,
                this));
    }

    void on_accept(
        beast::error_code ec,
        tcp::socket socket)
    {
        if (!ec)
        {
            std::make_shared<Session>(
                std::move(socket),
                pool_)
                ->run();
        }

        accept();
    }
};

int main()
{
    try
    {
        constexpr unsigned threads = 2;

        const std::string connectionString = getConnectionString();

        auto pool = std::make_shared<ConnectionPool>(
            connectionString,
            threads);

        asio::io_context ioc;

        Listener listener(
            ioc,
            tcp::endpoint(tcp::v4(), 8080),
            pool);

        listener.run();

        std::cout
            << "Boost.Beast listening on :8080 using "
            << threads
            << " worker threads\n";

        std::vector<std::thread> workers;
        workers.reserve(threads - 1);

        for (unsigned i = 1; i < threads; ++i)
        {
            workers.emplace_back([&ioc]()
                                 { ioc.run(); });
        }

        ioc.run();

        for (auto &t : workers)
            t.join();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}