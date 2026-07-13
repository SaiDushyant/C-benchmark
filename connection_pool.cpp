#include "connection_pool.h"

ConnectionPool::ConnectionPool(
    const std::string &connectionString,
    std::size_t poolSize)
{
    for (std::size_t i = 0; i < poolSize; ++i)
    {
        available_.push(
            std::make_shared<pqxx::connection>(
                connectionString));
    }
}

ConnectionPool::ConnectionGuard
ConnectionPool::acquire()
{
    std::unique_lock<std::mutex> lock(mutex_);

    cv_.wait(lock, [this]
             { return !available_.empty(); });

    auto conn = available_.front();
    available_.pop();

    return ConnectionGuard(*this, conn);
}

void ConnectionPool::release(
    std::shared_ptr<pqxx::connection> conn)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        available_.push(std::move(conn));
    }

    cv_.notify_one();
}

ConnectionPool::ConnectionGuard::~ConnectionGuard()
{
    if (conn_)
    {
        pool_.release(std::move(conn_));
    }
}