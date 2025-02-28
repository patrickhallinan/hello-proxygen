#pragma once

#include "HttpClient.h"

#include <memory>


namespace folly {
class EventBase;
}


class FeatureTest {
public:
    FeatureTest(folly::EventBase& eventBase);

    folly::Future<std::string> run();

private:
    folly::EventBase& eventBase_;
    folly::Promise<std::string> completed_;
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

