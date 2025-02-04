
add_executable(test
    HttpClient.cpp
    TransactionHandler.cpp
    Test.cpp

    feature_test.cpp
)


add_executable(performance
    HttpClient.cpp
    TransactionHandler.cpp

    performance_test.cpp
)


set(TARGET_LIBRARIES
    proxygenhttpserver
    proxygen
    wangle

    mvfst_exception

    folly
    fizz

    ${SODIUM_LDFLAGS}

    dl
    gflags
    glog
    pthread
    fmt
    OpenSSL::SSL
    lzma
    PkgConfig::lz4
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
    list(APPEND TARGET_LIBRARIES
        unwind
        iberty
    )
endif()


target_link_libraries(test
PRIVATE
    ${TARGET_LIBRARIES}
)


target_link_libraries(performance
PRIVATE
    ${TARGET_LIBRARIES}
)

