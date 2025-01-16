add_executable(hello
    HttpHandlerFactory.cpp
    EchoHandler.cpp
    HelloHandler.cpp

    main.cpp
)


find_package(bz2 QUIET)


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

    $<IF:$<BOOL:${bz2_FOUND}>,bz2,>

    boost_context
)

