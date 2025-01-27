#include "Test.h"

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>

#include <atomic>
#include <chrono>
#include <cstdlib>


DEFINE_string(target_host, "127.0.0.1", "IP address");
DEFINE_int32(target_port, 8080, "HTTP port");

DEFINE_int32(max_connections, 16, "should be a power of 2");
DEFINE_int32(payload_size, 2000, "number of bytes to send in each request");

static void performance_test(EventBase*);
static folly::Future<folly::Unit> sendRequests(std::vector<HttpClient*>&, std::string& payload);

// We really only need atomic if was have multiple event bases (i.e. threads)
std::atomic<int> requestCount(0);


// Dimensions
// ================
// Number of threads
// Number of clients
// Size of payload
// Echo or Ack
//
// Results
// ================
// Request/Second
// BPS


int main(int argc, char* argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;

    folly::Init _{&argc, &argv, /*removeFlags=*/false};

    folly::EventBase eventBase;

    performance_test(&eventBase);

    // test does not run until eventBase.loop()
    eventBase.loop();

    LOG(INFO) << "Exit main()\n";

    return 0;
}


void performance_test(EventBase* eventBase) {
    proxygen::HTTPHeaders headers;

    auto host = fmt::format("{}:{}", FLAGS_hello_host, FLAGS_hello_port);
    headers.add(proxygen::HTTP_HEADER_HOST, host);
    headers.add(proxygen::HTTP_HEADER_USER_AGENT, "test-client");
    headers.add(proxygen::HTTP_HEADER_ACCEPT, "*/*");

    std::vector<HttpClient*> clients;
    std::vector<folly::Future<folly::unit>> connectFutures;

    auto defaultTimeout = std::chrono::milliseconds(5000);

    auto url = fmt::format("http://{}:{}/", FLAGS_target_host, FLAGS_target_port);

    // create army of clients??
    for (int i = 0 ; i < FLAGS_max_connections ; i++) {
        clients[i] = new HttpClient(&eventBase,
                                    defaultTimeout,
                                    httpHeaders,
                                    url);

        connectFutures.pushback(clients[i]->connect());
    }

    static std::chrono::time_point<std::chrono::high_resolution_clock> start;
    static std::string payload

    payload.clear();
    for (int i = 0 ; i < FLAGS_payload_size ; i++)
        payload.push_back('a');


    // NOTE: This exits before before any of these events have happened.

    folly::collectAll(std::move(connectFutures))
        // folly::Unit is a standin for void, i.e. no value returned
        .then([&payload](std::vector<folly::Try<folly::Unit>> results) {
            for (const auto& result : results) {
                if (! result.hasValue()) {
                    LOG(ERROR) << "Exception: " << result.exception().what();
                    exit(1);
                }
            }


            start = std::chrono::high_resolution_clock::now();

            return sendRequests(clients, payload);
        })
        .then([&start](folly::Unit) {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;

            std::cout << "took " << duration.count() << " ms" << std::endl;
        });
}


folly::Future<HttpClient*> send(HttpClient* client, std::string& payload) {
    http.POST(payload)
        .thenValue([](const HttpResponse& response) {

        })
        .thenError(folly::tag_t<std::exception>{}, [](const std::exception& e) {
            LOG(INFO) << e.what();
            exit(1);
        });
}


folly::Future<folly::Unit> sendRequests(std::vector<HttpClient*>& clients, std::string& payload) {
    std::vector<folly::Future<HttpClient*> clientFutures;

    for (auto* client : clients) {
        clientFutures.push_back(send(client, payload));
    }

    return folly::collectAll(std::move(connectFutures));
}

