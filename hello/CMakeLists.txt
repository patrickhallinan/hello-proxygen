add_executable(hello
    HttpHandlerFactory.cpp
    EchoHandler.cpp
    HelloHandler.cpp

    main.cpp
)


target_link_libraries(hello
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


# for rocky linux
find_library(BZ2_LIB bz2)
if(BZ2_LIB)
target_link_libraries(hello
PUBLIC
    bz2
)
endif()

