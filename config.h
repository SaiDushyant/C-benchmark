#pragma once

#include <cstdlib>
#include <stdexcept>
#include <string>

inline std::string getEnv(const char *name)
{
    const char *value = std::getenv(name);

    if (!value)
    {
        throw std::runtime_error(
            std::string("Missing environment variable: ") + name);
    }

    return value;
}

inline std::string getConnectionString()
{
    return "host=" + getEnv("DB_HOST") +
           " port=" + getEnv("DB_PORT") +
           " dbname=" + getEnv("DB_NAME") +
           " user=" + getEnv("DB_USER") +
           " password=" + getEnv("DB_PASSWORD");
}