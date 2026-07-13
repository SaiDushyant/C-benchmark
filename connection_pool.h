#pragma once

#include <pqxx/pqxx>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

class ConnectionPool
{
public:
    class ConnectionGuard
    {
    public:
        ConnectionGuard(
            ConnectionPool &pool,
            std::shared_ptr<pqxx::connection> conn)
            : pool_(pool),
              conn_(std::move(conn))
        {
        }

        ConnectionGuard(const ConnectionGuard &) = delete;
        ConnectionGuard &operator=(const ConnectionGuard &) = delete;

        ConnectionGuard(ConnectionGuard &&other) noexcept
            : pool_(other.pool_),
              conn_(std::move(other.conn_))
        {
        }

        ~ConnectionGuard();

        pqxx::connection &operator*()
        {
            return *conn_;
        }

        pqxx::connection *operator->()
        {
            return conn_.get();
        }

    private:
        ConnectionPool &pool_;
        std::shared_ptr<pqxx::connection> conn_;
    };

public:
    ConnectionPool(
        const std::string &connectionString,
        std::size_t poolSize);

    ConnectionGuard acquire();

private:
    friend class ConnectionGuard;

    void release(std::shared_ptr<pqxx::connection> conn);

    std::mutex mutex_;
    std::condition_variable cv_;

    std::queue<std::shared_ptr<pqxx::connection>> available_;
};