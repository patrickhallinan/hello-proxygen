#include "feature_test.h"

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>


int main(int argc, char* argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;

    folly::Init _{&argc, &argv, /*removeFlags=*/false};

    folly::EventBase eventBase;

    FeatureTest test(eventBase);
    test.run()
        .thenValue([&eventBase](std::string&& result) {
            LOG(INFO) << result;
            eventBase.terminateLoopSoon();
        });

    // this drives the test and everything runs in this thread
    eventBase.loop();

    LOG(INFO) << "Exit main()\n";

    return 0;
}

