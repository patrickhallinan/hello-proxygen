
add_executable(test
    HttpClient.cpp
    TransactionHandler.cpp
    Test.cpp

    main.cpp
)


find_package(bz2 QUIET)


target_link_libraries(test
PUBLIC
    proxygenhttpserver
    proxygen
    wangle
    fizz
    folly

    unwind
    sodium

    dl
    gflags
    glog
    pthread
    fmt
    ssl
    lzma
    lz4
    snappy
    crypto
    z
    zstd
    iberty
    event
    double-conversion

    $<IF:$<BOOL:${bz2_FOUND}>,bz2,>

    boost_context
)

