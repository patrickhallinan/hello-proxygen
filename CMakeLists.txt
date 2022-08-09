project (HelloProxygen)
cmake_minimum_required(VERSION 3.16)

set(GCC_COMPILE_FLAGS "-Wfatal-errors")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}")

set(CMAKE_PREFIX_PATH
    "proxygen/proxygen/_build/"
)

include_directories(
PUBLIC
    proxygen/
    proxygen/proxygen/_build/deps/include
    proxygen/proxygen/_build/generated/
)

link_directories(
    proxygen/proxygen/_build/deps/lib/
    proxygen/proxygen/_build/proxygen/lib/
    proxygen/proxygen/_build/proxygen/lib/http/
    proxygen/proxygen/_build/proxygen/lib/services/
    proxygen/proxygen/_build/proxygen/lib/utils/
    proxygen/proxygen/_build/proxygen/httpserver/
)


add_executable(Hello
    main.cpp
    HttpHandlerFactory.cpp
    HttpHandler.cpp
)

target_link_libraries(Hello
PUBLIC
    proxygenhttpserver
    proxygen
    wangle
    fizz
    folly

    unwind
    sodium

    dl
    event
    iberty
    pthread
    fmt
    glog
    gflags
    ssl
    lzma
    lz4
    snappy
    crypto
    z
    zstd
    double-conversion

    boost_context
)

