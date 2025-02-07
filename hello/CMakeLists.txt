add_executable(hello
    HttpHandlerFactory.cpp
    EchoHandler.cpp
    HelloHandler.cpp

    main.cpp
)


set(TARGET_LIBRARIES
    proxygenhttpserver
	httpclient

    proxygen
    wangle

    mvfst_exception

    folly
    fizz

    ${SODIUM_LDFLAGS}

    dl
    ${LIBEVENT_LIBRARIES}
    pthread
    fmt
    glog
    gflags
    lzma
    ${SNAPPY_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto

    PkgConfig::lz4

    z
    zstd
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


# for rocky linux
find_library(BZ2_LIB bz2)
if(BZ2_LIB)
    list(APPEND TARGET_LIBRARIES bz2)
endif()


target_link_libraries(hello
PRIVATE
    ${TARGET_LIBRARIES}
)
