#include <uwebsockets/App.h>

#include "connection_pool.h"

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "config.h"

int main()
{
    constexpr unsigned threads = 2;

    const std::string connectionString = getConnectionString();

    auto pool = std::make_shared<ConnectionPool>(
        connectionString,
        threads);

    std::cout
        << "Starting "
        << threads
        << " uWebSockets workers\n";

    std::vector<std::thread> workers;
    workers.reserve(threads);

    for (unsigned i = 0; i < threads; ++i)
    {
        workers.emplace_back([pool, i]()
                             { uWS::App()

                                   .get("/", [pool](auto *res, auto *)
                                        {
                    std::string body;

                    try
                    {
                        auto conn = pool->acquire();

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

                    res->writeStatus("200 OK")
                        ->writeHeader("Server", "benchmark")
                        ->writeHeader("Content-Type", "application/json")
                        ->end(body); })

                                   .listen(
                                       8080,
                                       [i](auto *listenSocket)
                                       {
                                           if (listenSocket)
                                           {
                                               std::cout
                                                   << "Worker "
                                                   << i
                                                   << " listening\n";
                                           }
                                           else
                                           {
                                               std::cout
                                                   << "Worker "
                                                   << i
                                                   << " failed\n";
                                           }
                                       })

                                   .run(); });
    }

    for (auto &t : workers)
        t.join();

    return 0;
}