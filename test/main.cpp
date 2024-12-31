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

    //google::InitGoogleLogging(argv[0]); called by folly::Init
    folly::Init _{&argc, &argv, /*removeFlags=*/false};

    //auto eventBase = std::make_unique<folly::EventBase>();
    folly::EventBase eventBase;

    // the timer is used for timing events in the event loop.  It must be
    // initialized before starting the loop
    auto defaultTimeout = std::chrono::milliseconds(5000);
    proxygen::WheelTimerInstance timer{defaultTimeout, &eventBase};

    auto eventThread = std::thread([&eventBase](){
        LOG(INFO) << "HTTP Client Event Handler thread started";
        eventBase.loop();
        //eventBase.loopForever();
    });

    //eventBase->runInEventBaseThreadAndWait([&]() {

    Test test(eventBase, timer);

    eventThread.join();

    //
    //eventBase.loopForever();

    /*
    auto eventThread = std::thread([eventBase=eventBase.get()](){
        LOG(INFO) << "HTTP Client Event Handler thread started";
        eventBase->loopForever();
    });
    eventThread.detach();
    eventThread.join();
    */

    std::cout << "Exit main()\n";

    return 0;
}


