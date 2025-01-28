#include "Test.h"

#include "HttpClient.h"

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>

#include <atomic>
#include <chrono>
#include <cstdlib>


DEFINE_string(target_host, "127.0.0.1", "IP address");
DEFINE_int32(target_port, 8080, "HTTP port");

DEFINE_int32(num_connections, 16, "should be a power of 2");
DEFINE_int32(number_of_requests, 1000, "number of http requests");
DEFINE_int32(payload_size, 2000, "number of bytes to send in each request");

void performance_test(folly::EventBase*);

folly::Future<folly::Unit> sendRequests(folly::EventBase* eventBase,
                                        std::vector<HttpClient*> clients,
                                        std::string& payload);

// We really only need atomic if was have multiple event bases (i.e. threads)
std::atomic<int> requestCount(0);


// Dimensions
// ================
// Number of threads - not implemented
// Number of clients (i.e. number of connections) - not implemented
// Size of payload - FLAGS_payload_size
// Echo or Ack - not implemented
//
// Results
// ================
// Request time
// Request rate
// Bit rate


/*
static void nacho(folly::EventBase* eb) {
    using namespace folly;

    eb->runInEventBaseThread([eb]() {
        auto p = std::make_shared<Promise<folly::Unit>>();

        LOG(INFO) << "NACHO";

        auto defaultTimeout = std::chrono::milliseconds(500);
        proxygen::WheelTimerInstance timer{defaultTimeout, eb};

        return p->getFuture()
            //.within(std::chrono::milliseconds(500), &eb->timer())
            //.within(std::chrono::milliseconds(500), std::runtime_error("Salsa"))
            .within(std::chrono::milliseconds(500), timer, std::runtime_error("Salsa"))
            .thenValue([](folly::Unit) {
                LOG(INFO) << ".thenValue()";
            })
            //.thenError(folly::tag_t<folly::OperationCancelled>{}, [p](const folly::OperationCancelled& e) {
            .thenError(folly::tag_t<std::exception>{}, [p](const std::exception& e) {
                LOG(INFO) << ".thenError()  TACO";
                LOG(INFO) << e.what();
                p->setValue(folly::unit);
            });
    });
}
*/


int main(int argc, char* argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;

    folly::Init _{&argc, &argv, /*removeFlags=*/false};

    folly::EventBase eventBase;

    performance_test(&eventBase);
    //nacho(&eventBase);

    // test does not run until eventBase.loop()
    eventBase.loop();
    //eventBase.loopForever();

    LOG(INFO) << "Exit main()\n";

    return 0;
}


folly::Future<HttpClient*> send(HttpClient* client, std::string& payload) {
    if (client == nullptr) {
        LOG(INFO) << "Nacho";
    }

    return client->POST(payload)
        .thenValue([client, &payload](const HttpResponse& response) -> folly::Future<HttpClient*> {
            if (++requestCount < FLAGS_number_of_requests)
                return send(client, payload);
            return folly::makeFuture(client);
        })
        .thenError(folly::tag_t<std::exception>{}, [&](const std::exception& e) {
            LOG(INFO) << e.what();
            exit(1);
            return folly::makeFuture(client);
        });
}


folly::Future<folly::Unit> sendRequests(folly::EventBase* eventBase,
                                        std::vector<HttpClient*> clients,
                                        std::string& payload) {

    std::vector<folly::Future<HttpClient*>> clientFutures;

    for (auto* client : clients) {
        clientFutures.push_back(send(client, payload));
    }

    return folly::collectAll(std::move(clientFutures))
        .via(eventBase)
        .thenValue([](folly::SemiFuture<std::vector<folly::Try<HttpClient *>>>) {
            return folly::unit;
        });
}


void performance_test(folly::EventBase* eventBase) {
    proxygen::HTTPHeaders headers;

    auto host = fmt::format("{}:{}", FLAGS_target_host, FLAGS_target_port);
    headers.add(proxygen::HTTP_HEADER_HOST, host);
    headers.add(proxygen::HTTP_HEADER_USER_AGENT, "test-client");
    headers.add(proxygen::HTTP_HEADER_ACCEPT, "*/*");

    std::vector<HttpClient*> clients;
    std::vector<folly::Future<folly::Unit>> connectFutures;

    auto defaultTimeout = std::chrono::milliseconds(5000);

    auto url = fmt::format("http://{}:{}/", FLAGS_target_host, FLAGS_target_port);

    for (int i = 0 ; i < FLAGS_num_connections ; i++) {
        clients.push_back(new HttpClient(eventBase,
                                         defaultTimeout,
                                         headers,
                                         url));

        connectFutures.push_back(clients[i]->connect());
    }

    static std::chrono::time_point<std::chrono::high_resolution_clock> start;
    static std::string payload;

    payload.clear();
    for (int i = 0 ; i < FLAGS_payload_size ; i++)
        payload.push_back('a');


    // NOTE: This exits before before any of these events have happened.

    folly::collectAll(std::move(connectFutures))
        .via(eventBase)
        .thenValue([eventBase, clients](std::vector<folly::Try<folly::Unit>> results) mutable {
            for (const auto& result : results) {
                if (! result.hasValue()) {
                    LOG(ERROR) << "Exception: " << result.exception().what();
                    exit(1);
                }
            }

            LOG(INFO) << "All HttpClient's connected";

            start = std::chrono::high_resolution_clock::now();

            return sendRequests(eventBase, std::move(clients), payload);
        })
        .thenValue([clients](folly::Unit) {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;

            std::cout << "took " << duration.count() << " ms" << std::endl;

            for (auto& client : clients)
                delete client;
        });
}

