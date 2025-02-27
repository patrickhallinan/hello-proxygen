
add_executable(ft
    ft_main.cpp
    feature_test.cpp
)


add_executable(perf
    perf_main.cpp
    performance_test.cpp
)


add_executable(test
    test_server_main.cpp
    feature_test.cpp
    performance_test.cpp
)


set(TARGET_LIBRARIES
    httpclient

    proxygen
    wangle

    mvfst_exception

    folly
    fizz

    ${SODIUM_LDFLAGS}

    dl
    pthread
    fmt
    gflags
    glog

    OpenSSL::SSL
    lzma
    OpenSSL::Crypto

    PkgConfig::lz4

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


target_link_libraries(ft
PRIVATE
    ${TARGET_LIBRARIES}
)


target_link_libraries(perf
PRIVATE
    ${TARGET_LIBRARIES}
)


# for rocky linux
find_library(BZ2_LIB bz2)
if(BZ2_LIB)
    list(APPEND TARGET_LIBRARIES bz2)
endif()


target_link_libraries(test
PRIVATE
    proxygenhttpserver

    ${TARGET_LIBRARIES}
    ${SNAPPY_LIB}
)

