#pragma once

#include <folly/futures/Future.h>

#include <string>
#include <vector>


namespace folly {
class EventBase;
}


struct PerformanceTestParams {
    std::string target_host;
    uint16_t target_port;

    int32_t number_of_connections;
    int32_t number_of_requests;
    int32_t payload_size;
};


class PerformanceTest {
    folly::EventBase* eventBase_;
    const PerformanceTestParams params_;
    const std::string payload_;

public:
    PerformanceTest(folly::EventBase* eventBase,
                    const PerformanceTestParams& params);

    folly::Future<std::vector<std::string>> run();
};
