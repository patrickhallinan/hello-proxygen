#include "Test.h"

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>


DEFINE_string(hello_host, "127.0.0.1", "IP address");
DEFINE_int32(hello_port, 8080, "HTTP port");


int main(int argc, char* argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;

    folly::Init _{&argc, &argv, /*removeFlags=*/false};

    folly::EventBase eventBase;

    Test test(eventBase);
    test.run();

    // XXX: Why does this wait 1m 12s after finishing the test?
    eventBase.loop();

    LOG(INFO) << "Exit main()\n";

    return 0;
}


