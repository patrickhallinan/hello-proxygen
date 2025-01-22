
add_executable(test
    HttpClient.cpp
    TransactionHandler.cpp
    Test.cpp

    main.cpp
)


set(TARGET_LIBRARIES
    proxygenhttpserver
    proxygen
    wangle
    fizz
    folly

    ${SODIUM_LDFLAGS}

    dl
    gflags
    glog
    pthread
    fmt
    OpenSSL::SSL
    lzma
    PkgConfig::lz4
    ${SNAPPY_LIB}
    OpenSSL::Crypto
    z
    zstd
    ${LIBEVENT_LIBRARIES}
    double-conversion::double-conversion

    Boost::context
)


if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    list(APPEND TARGET_LIBRARIES
        c++abi
    )
else()
    #list(APPEND TARGET_LIBRARIES boost-context)
    list(APPEND TARGET_LIBRARIES
        unwind
        iberty
    )
endif()


target_link_libraries(test
PRIVATE
    ${TARGET_LIBRARIES}
)

