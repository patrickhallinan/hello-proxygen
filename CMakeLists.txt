cmake_minimum_required(VERSION 3.16)

project (HelloProxygen)

# for rtags
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the C++ standard to C++20 for all targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GCC_COMPILE_FLAGS "-Wfatal-errors -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}")

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")

set(CMAKE_PREFIX_PATH
    "proxygen/proxygen/_build/"
)

# Suppress warnings for headers included
# add_compile_options(-isystem /usr/include/openssl)
include_directories(SYSTEM proxygen/proxygen/_build/deps/include)

include_directories(
PUBLIC
    proxygen/
    proxygen/proxygen/_build/deps/include
    proxygen/proxygen/_build/generated/
)


link_directories(
    proxygen/proxygen/_build/deps/lib/
    proxygen/proxygen/_build/deps/lib64/
    proxygen/proxygen/_build/proxygen/lib/
    proxygen/proxygen/_build/proxygen/lib/http/
    proxygen/proxygen/_build/proxygen/lib/services/
    proxygen/proxygen/_build/proxygen/lib/utils/
    proxygen/proxygen/_build/proxygen/httpserver/
)


add_subdirectory(hello)
add_subdirectory(test)
