cmake_minimum_required(VERSION 3.16)

project (HelloProxygen)

# for rtags
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
else()
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(CMAKE_CXX_COMPILER "clang++")
    set(CMAKE_C_COMPILER "clang")
endif()


set(GCC_COMPILE_FLAGS "-Wfatal-errors -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}")

set(CMAKE_PREFIX_PATH
    "proxygen/proxygen/_build/"
)

# Suppress warnings for headers included
#include_directories(SYSTEM proxygen/proxygen/_build/deps/include)


find_library(SNAPPY_LIB snappy)


find_package(Boost REQUIRED)
find_package(Boost REQUIRED COMPONENTS context)
find_package(double-conversion REQUIRED)
find_package(gflags REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(GLOG REQUIRED libglog)
pkg_check_modules(lz4 REQUIRED IMPORTED_TARGET liblz4)
pkg_check_modules(SODIUM REQUIRED libsodium)


list(APPEND CMAKE_SYSTEM_LIBRARY_PATH
    /usr/local/lib
    # XXX IS THIS DOING ANYTHING???
    #${GLOG_LIBRARY_DIRS}
)


include_directories(
PUBLIC
    proxygen/
    proxygen/proxygen/_build/generated/
SYSTEM
    proxygen/proxygen/_build/deps/include
    ${Boost_INCLUDE_DIRS}
    ${gflags_INCLUDE_DIR}
    ${GLOG_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${SODIUM_INCLUDE_DIRS}
    ${LIBEVENT_INCLUDE_DIRS}
)


link_directories(
    proxygen/proxygen/_build/deps/lib/
    proxygen/proxygen/_build/deps/lib64/
    proxygen/proxygen/_build/proxygen/lib/
    proxygen/proxygen/_build/proxygen/lib/http/
    proxygen/proxygen/_build/proxygen/lib/services/
    proxygen/proxygen/_build/proxygen/lib/utils/
    proxygen/proxygen/_build/proxygen/httpserver/

    ${Boost_LIBRARY_DIRS}
    ${GLOG_LIBRARY_DIRS}
    ${LIBEVENT_LIBRARY_DIRS}
)


add_subdirectory(hello)
add_subdirectory(test)
