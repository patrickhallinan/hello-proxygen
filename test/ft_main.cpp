#include "feature_test.h"

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>


DEFINE_string(hello_host, "127.0.0.1", "IP address");
DEFINE_int32(hello_port, 8080, "HTTP port");


int main(int argc, char* argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;

    folly::Init _{&argc, &argv, /*removeFlags=*/false};

    folly::EventBase eventBase;

    FeatureTest test(eventBase, FLAGS_hello_host, FLAGS_hello_port);

    test.run()
        .thenValue([&eventBase](FeatureTestResult&& result) {
            LOG(INFO) << result.result << "  " << result.msg;
            eventBase.terminateLoopSoon();
        });

    // this drives the test and everything runs in this thread
    eventBase.loop();

    LOG(INFO) << "Exit main()\n";

    return 0;
}

