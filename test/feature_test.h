#pragma once

#include "HttpClient.h"

#include <memory>


namespace folly {
class EventBase;
}


struct FeatureTestResult {
    std::string result;
    std::string msg;
};


class FeatureTest {
public:
    FeatureTest(folly::EventBase& eventBase,
                const std::string& host,
                uint16_t port);

    folly::Future<FeatureTestResult> run();

private:
    folly::EventBase& eventBase_;
    std::string host_;
    uint16_t port_;

    folly::Promise<FeatureTestResult> completed_;
    std::unique_ptr<HttpClient> httpClient_;
    std::string failMsg_;
    bool passed_{true};
};


#define  assert_equal(a,b) { \
    if ((a) != (b)) { \
        std::string msg =  fmt::format("Failed: '{}' != '{}'  -> {}:{}", \
            (a), (b), __FILE__, __LINE__); \
        throw std::runtime_error(msg); \
    } \
}

