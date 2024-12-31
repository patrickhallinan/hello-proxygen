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
    std::unique_ptr<HttpClient> httpClient_;
};
