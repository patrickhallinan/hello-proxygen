#pragma once

#include "HttpClient.h"

#include <memory>


namespace folly {
class EventBase;
}

namespace proxygen {
class WheelTimerInstance;
}


class Test {
public:
    Test(folly::EventBase& eventBase, proxygen::WheelTimerInstance& timer);

    void run();

private:
    folly::EventBase& eventBase_;
    proxygen::WheelTimerInstance& timer_;

    std::unique_ptr<HttpClient> httpClient_;
};
