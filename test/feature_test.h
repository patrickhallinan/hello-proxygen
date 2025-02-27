#pragma once

#include "HttpClient.h"

#include <memory>


namespace folly {
class EventBase;
}


class Test {
public:
    Test(folly::EventBase& eventBase);

    void run();

private:
    folly::EventBase& eventBase_;
    std::unique_ptr<HttpClient> httpClient_;
};


#define  assert_equal(a,b) { \
    if ((a) != (b)) { \
        std::string msg =  fmt::format("Failed: '{}' != '{}'  -> {}:{}", \
            (a), (b), __FILE__, __LINE__); \
        throw std::runtime_error(msg); \
    } \
}
