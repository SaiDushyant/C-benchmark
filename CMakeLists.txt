cmake_minimum_required(VERSION 3.20)

project(HttpBenchmark)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(boost_beast CONFIG REQUIRED)
find_package(unofficial-uwebsockets CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

add_executable(beast_server
    beast_server.cpp
    connection_pool.cpp
)

target_link_libraries(beast_server
    PRIVATE
    Boost::system
    Boost::beast
    libpqxx::pqxx
)

add_executable(uws_server
    uws_server.cpp
    connection_pool.cpp
)

target_link_libraries(uws_server
    PRIVATE
    unofficial::uwebsockets::uwebsockets
    libpqxx::pqxx
)

add_executable(seed_db
    seed_db.cpp
)

target_link_libraries(seed_db
    PRIVATE
    libpqxx::pqxx
)