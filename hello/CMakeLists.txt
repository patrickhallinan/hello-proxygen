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

    #unwind
    #sodium
    ${SODIUM_LDFLAGS}

    dl
    #event
    ${LIBEVENT_LIBRARIES}
    #iberty
    pthread
    fmt
    glog
    gflags
    #ssl
    lzma
    #lz4
    #snappy
    ${SNAPPY_LIB}
    #crypto
    OpenSSL::SSL
    OpenSSL::Crypto

    #lz4
    PkgConfig::lz4

    z
    zstd
    double-conversion::double-conversion

    #boost_context
    Boost::context

    c++abi
)




# for rocky linux
find_library(BZ2_LIB bz2)
if(BZ2_LIB)
target_link_libraries(hello
PUBLIC
    bz2
)
endif()

