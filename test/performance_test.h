#pragma once

#include <folly/futures/Future.h>

#include <atomic>
#include <string>
#include <vector>


namespace folly {
class EventBase;
}

class HttpClient;


struct PerformanceTestParams {
    std::string target_host;
    uint16_t target_port;

    int32_t number_of_connections;
    int32_t number_of_requests;
    int32_t payload_size;
};


class PerformanceTest : PerformanceTestParams {
public:
    PerformanceTest(folly::EventBase* eventBase,
                    const PerformanceTestParams& params)
        : PerformanceTestParams{params}
        , eventBase_{eventBase}
    {
        // TODO: check PerformanceTestParams and set to default values if not set.
    }

    folly::Future<std::vector<std::string>> run();

private:
    folly::Future<HttpClient*> send(HttpClient* client);

    folly::Future<folly::Unit> sendRequests();

    folly::Future<folly::Unit> connect(int clientIndex=0, int retries=0);

    folly::Future<folly::Unit> connectClients();

    std::vector<std::string> results(double total_time_ms);

    folly::EventBase* eventBase_;

    std::vector<HttpClient*> clients_;
    std::string payload_;
    std::string errorMessage_;

    // We really only need atomic<> if was have multiple event bases (i.e. threads)
    std::atomic<int> requestCount{0};
};
