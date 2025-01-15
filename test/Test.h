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
