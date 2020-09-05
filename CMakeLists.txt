cmake_minimum_required(VERSION 3.10)

project(static_server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.65 REQUIRED
    COMPONENTS system program_options thread filesystem coroutine log log_setup)

set(CMAKE_CXX_FLAGS "-W -Wall -Wextra -Werror -fPIC -pthread -fno-omit-frame-pointer -std=c++17 -ggdb")

add_subdirectory(src)