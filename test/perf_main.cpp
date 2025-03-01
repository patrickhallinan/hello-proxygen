#include "performance_test.h"

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>


int main(int argc, char* argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;

    folly::Init _{&argc, &argv, /*removeFlags=*/false};

    folly::EventBase eventBase;

    performance_test(&eventBase)
        .thenValue([&eventBase](std::vector<std::string>&& lines) {

            for (const auto& line : lines)
                LOG(INFO) << line;

            eventBase.terminateLoopSoon();
        });

    eventBase.loop();

    LOG(INFO) << "Exit main()\n";

    return 0;
}

