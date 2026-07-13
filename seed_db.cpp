#include <pqxx/pqxx>

#include <iostream>
#include <string>

int main()
{
    try
    {
        const std::string connectionString =
            "host=localhost "
            "port=5432 "
            "dbname=benchmark "
            "user=xtremenext_viji";

        pqxx::connection conn(connectionString);

        if (!conn.is_open())
        {
            std::cerr << "Failed to connect to PostgreSQL\n";
            return 1;
        }

        std::cout << "Connected to PostgreSQL\n";

        pqxx::work tx(conn);

        tx.exec(R"(
            DROP TABLE IF EXISTS users;

            CREATE TABLE users
            (
                id SERIAL PRIMARY KEY,
                name TEXT NOT NULL,
                email TEXT NOT NULL
            );
        )");

        std::cout << "Creating users...\n";

        for (int i = 1; i <= 10000; ++i)
        {
            tx.exec_params(
                "INSERT INTO users(name, email) VALUES($1, $2)",
                "User " + std::to_string(i),
                "user" + std::to_string(i) + "@example.com");
        }

        tx.commit();

        std::cout << "Inserted 10000 users.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}