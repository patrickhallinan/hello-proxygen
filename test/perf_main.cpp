#include "performance_test.h"

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>


DEFINE_string(target_host, "127.0.0.1", "IP address");
DEFINE_int32(target_port, 8080, "HTTP port");

DEFINE_int32(num_connections, 16, "should be a power of 2");
DEFINE_int32(number_of_requests, 100, "number of http requests");
DEFINE_int32(payload_size, 2000, "number of bytes to send in each request");


int main(int argc, char* argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;

    folly::Init _{&argc, &argv, /*removeFlags=*/false};

    folly::EventBase eventBase;

    PerformanceTestParams params{ .target_host=FLAGS_target_host,
                                  .target_port=(uint16_t)FLAGS_target_port,
                                  .number_of_connections=FLAGS_num_connections,
                                  .number_of_requests=FLAGS_number_of_requests,
                                  .payload_size=FLAGS_payload_size };

    PerformanceTest test{&eventBase, params};

    test.run()
        .thenValue([](std::vector<std::string>&& lines) {

            for (const auto& line : lines)
                LOG(INFO) << line;
        })
        .thenError(folly::tag_t<std::exception>{}, [](const std::exception&& e) {

            LOG(ERROR) << e.what();
        })
        .ensure([&eventBase]() {
            eventBase.terminateLoopSoon();
        });


    eventBase.loop();

    LOG(INFO) << "Exit main()\n";

    return 0;
}

