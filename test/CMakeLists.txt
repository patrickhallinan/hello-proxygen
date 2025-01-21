
add_executable(test
    HttpClient.cpp
    TransactionHandler.cpp
    Test.cpp

    main.cpp
)


target_link_libraries(test
PUBLIC
    proxygenhttpserver
    proxygen
    wangle
    fizz
    folly

    #unwind
    #sodium
    ${SODIUM_LDFLAGS}

    dl
    gflags
    glog
    pthread
    fmt
    #ssl
    OpenSSL::SSL
    lzma
    #lz4
    PkgConfig::lz4
    #snappy
    ${SNAPPY_LIB}
    #crypto
    OpenSSL::Crypto
    z
    zstd
    #iberty
    event
    ${LIBEVENT_LIBRARIES}
    #double-conversion
    double-conversion::double-conversion

    #boost_context
    Boost::context

    c++abi
)

