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
    bz2

    boost_context
)

