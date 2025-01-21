cmake_minimum_required(VERSION 3.16)

project (HelloProxygen)

# for rtags
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Detect if the system is macOS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Optionally, for macOS, specify the C++ standard library
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

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


find_package(PkgConfig REQUIRED)


find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()


pkg_check_modules(lz4 REQUIRED IMPORTED_TARGET liblz4)


find_package(gflags REQUIRED)
if (gflags_FOUND)
    include_directories(${gflags_INCLUDE_DIR})
else()
    message(FATAL_ERROR "gflags not found")
endif()


find_library(SNAPPY_LIB snappy)

message("SNAPPY_LIBRARIES:  ${SNAPPY_LIB}")

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})


find_package(Boost REQUIRED COMPONENTS context)


pkg_check_modules(GLOG REQUIRED libglog)
if (GLOG_FOUND)
    include_directories(${GLOG_INCLUDE_DIRS})
    link_directories(${GLOG_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "glog not found")
endif()

pkg_check_modules(SODIUM REQUIRED libsodium)

list(APPEND CMAKE_SYSTEM_LIBRARY_PATH
    /usr/local/lib
    ${GLOG_LIBRARY_DIRS}
)

pkg_check_modules(LIBEVENT REQUIRED libevent)

#foreach(LIB_DIR ${LIBEVENT_LIBRARY_DIRS})
#    link_directories(${LIB_DIR})
#endforeach()

find_package(double-conversion REQUIRED)

include_directories(
PUBLIC
    proxygen/
    proxygen/proxygen/_build/generated/
SYSTEM
    proxygen/proxygen/_build/deps/include
    ${GLOG_INCLUDE_DIRS}
    ${SODIUM_INCLUDE_DIRS}
    ${LIBEVENT_INCLUDE_DIRS}
)


link_directories(
    proxygen/proxygen/_build/deps/lib/
    #proxygen/proxygen/_build/deps/lib64/
    proxygen/proxygen/_build/proxygen/lib/
    proxygen/proxygen/_build/proxygen/lib/http/
    proxygen/proxygen/_build/proxygen/lib/services/
    proxygen/proxygen/_build/proxygen/lib/utils/
    proxygen/proxygen/_build/proxygen/httpserver/

    ${LIBEVENT_LIBRARY_DIRS}
    ${GLOG_LIBRARY_DIRS}
)



add_subdirectory(hello)
add_subdirectory(test)
