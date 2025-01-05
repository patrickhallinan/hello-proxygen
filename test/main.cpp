#include "Test.h"

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>

#include <proxygen/lib/utils/WheelTimerInstance.h>

#include <fmt/core.h>
#include <chrono>
#include <iostream>


DEFINE_string(hello_host, "127.0.0.1", "IP address");
DEFINE_int32(hello_port, 8080, "HTTP port");

using namespace std::chrono_literals;


int main(int argc, char* argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;

    folly::Init _{&argc, &argv, /*removeFlags=*/false};

    folly::EventBase eventBase;

    auto defaultTimeout = std::chrono::milliseconds(5000);
    proxygen::WheelTimerInstance timer{defaultTimeout, &eventBase};

    // XXX: Need to nail down the initialization requirements
    Test test(eventBase, timer);
    test.run();

    auto eventThread = std::thread([&eventBase](){
        LOG(INFO) << "Event loop thread started";
        eventBase.loop();
        //eventBase.loopForever();
    });


    eventThread.join();

    std::cout << "Exit main()\n";

    return 0;
}


