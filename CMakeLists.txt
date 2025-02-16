cmake_minimum_required(VERSION 3.16)

project (HelloProxygen)

# for rtags
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")

set(GCC_COMPILE_FLAGS "-Wfatal-errors -Werror -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}")


# We are getting a warning for using findBoost() on some platforms
# We should not because we are not using findBoost()???
# This suppresses the warning.
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

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


include_directories(
SYSTEM
    proxygen/
    proxygen/proxygen/_build/generated/
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
    proxygen/proxygen/_build/proxygen/lib/
    proxygen/proxygen/_build/proxygen/lib/http/
    proxygen/proxygen/_build/proxygen/lib/services/
    proxygen/proxygen/_build/proxygen/lib/utils/
    proxygen/proxygen/_build/proxygen/httpserver/

    ${Boost_LIBRARY_DIRS}
    ${GLOG_LIBRARY_DIRS}
    ${LIBEVENT_LIBRARY_DIRS}
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/proxygen/proxygen/_build/deps/lib64/")
    link_directories("${CMAKE_CURRENT_SOURCE_DIR}/proxygen/proxygen/_build/deps/lib64/")
endif()


add_subdirectory(lib)
add_subdirectory(hello)
add_subdirectory(test)
