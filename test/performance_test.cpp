#include "performance_test.h"

#include "HttpClient.h"

#include <fmt/format.h>
#include <folly/futures/ThreadWheelTimekeeper.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>

#include <chrono>
#include <random>
#include <stdexcept>
#include <string_view>


namespace {


proxygen::HTTPHeaders httpHeaders() {
    proxygen::HTTPHeaders headers;

    headers.add(proxygen::HTTP_HEADER_USER_AGENT, "performance-test");
    headers.add(proxygen::HTTP_HEADER_CACHE_CONTROL, "no-store");
    headers.add(proxygen::HTTP_HEADER_ACCEPT, "*/*");

    return headers;
}


std::string makePayload(int size) {
    std::string payload;

    static const std::string characters{ "abcdefghijklmnopqrstuvwxyz"
                                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "0123456789" };

    std::mt19937 random_number_generator{ std::random_device{}() };
    std::uniform_int_distribution<int> distribution(0, characters.size() - 1);

    for (int i = 0 ; i < size ; i++)
        payload += characters[distribution(random_number_generator)];

    return payload;
}

} // end namespace anon


folly::Future<HttpClient*> PerformanceTest::send(HttpClient* client) {

    if (++requestCount > number_of_requests)
        return folly::makeFuture(client);

    return client->POST("/feature-test", payload_)
        .thenValue([this, client](const HttpResponse& response) {

            if (response.status() != 200) {
                auto msg = fmt::format("received invalid status code from host: {}",
                        response.status());
                throw std::runtime_error(msg);
            }

            else if (response.content() != payload_) {
                auto truncate = [](std::string_view s, size_t N) {
                    return s.substr(0, std::min(s.length(), N));
                };

                auto msg = fmt::format("invalid response from host. expected size={} "
                        ", actual size={}, content:\n{}", payload_.size(),
                        response.content().size(), truncate(response.content(), 200));

                throw std::runtime_error(msg);
            }

            return send(client);
        });
}


// This is an artifact of earlier developement and may not make sense
// as a separate function (may have never made sense).
folly::Future<folly::Unit> PerformanceTest::sendRequests() {

    std::vector<folly::Future<HttpClient*>> clientFutures;

    for (auto* client : clients_) {
        // send() is poorly named.  each client will continually send/receive
        // a payload until the number_of_requests limit is reached.
        clientFutures.push_back(send(client));
    }

    return folly::collectAll(std::move(clientFutures))
        .via(eventBase_)
        .thenValue([](folly::SemiFuture<std::vector<folly::Try<HttpClient *>>>) {
            return folly::unit;
        });
}


// Originally client->connect() was called on all the clients and folly::collectAll()
// was used to wait for them all to connect but occassionally client->connect() failed
// with a timeout exception.  This work's around the issue by connecting clients one
// by one and doing retries when an exception occurs.
//
// This could be done faster by connecting in batches and retrying connection failures
// in subsequent batches
//
folly::Future<folly::Unit> PerformanceTest::connect(int index, int retries) {

    // TODO: Make max consecutive retries a test parameter

    static constexpr int max_retries = 3;

    if (retries >= max_retries) {
        auto msg = fmt::format("Connect to '{}:{}' failed  Exceeded max retries: {}",
            target_host, target_port, max_retries);
        throw std::runtime_error{msg};
    }

    if (index >= clients_.size())
        return folly::unit;

    HttpClient* client = clients_[index];

    return client->connect(target_host, target_port)
        .thenValue([this, index, retries](folly::Unit) {
            return connect(index+1, 0);
        })
        .thenError(folly::tag_t<std::exception>{},
            [this, index, retries](const std::exception& e) mutable {

            LOG(ERROR) << e.what();

            return connect(index, retries+1);
        });
}


folly::Future<folly::Unit> PerformanceTest::connectClients() {

    using namespace std::chrono_literals;

    auto endpoint = fmt::format("{}:{}", target_host, target_port);

    LOG(INFO) << "Connecting to: " << endpoint;

    auto start = std::chrono::high_resolution_clock::now();

    return connect()
        .thenValue([this, start] (folly::Unit) {
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> elapsed = end - start;

            LOG(INFO) << "Connected " << clients_.size() << " HttpClient's in "
                << elapsed.count() << " seconds";

            return folly::unit;
        });
}


std::vector<std::string> PerformanceTest::results(double total_time_ms) {

    double seconds = total_time_ms / 1000.0;
    double averageRequestTime = total_time_ms / number_of_requests;
    double requestRate = number_of_requests / seconds;
    double mbps = requestRate * 8 * payload_size / (1024*10240);

    std::vector<std::string> lines;

#define f fmt::format

    lines.push_back(f("sent {} HTTP POST requests", number_of_requests));
    lines.push_back(f("total time: {} seconds", seconds));
    lines.push_back(f("content size: {} bytes", payload_size));
    lines.push_back(f("num connections: {} connections", number_of_connections));
    lines.push_back(f("avg request Time: {} ms", averageRequestTime));
    lines.push_back(f("request rate: {} requests/second", requestRate));
    lines.push_back(f("bit rate: {} Mbps", mbps));

#undef f

    return lines;
}


folly::Future<std::vector<std::string>> PerformanceTest::run() {

    using namespace std;
    using namespace folly;

    auto promise = make_shared< Promise<vector<string>> >();

    payload_ = makePayload(payload_size);

    for (int i = 0 ; i < number_of_connections ; i++)
        clients_.push_back(new HttpClient(eventBase_, 5s, httpHeaders()));

    connectClients()
        .thenValue([this, promise](folly::Unit) {

            auto start = std::chrono::high_resolution_clock::now();

            return sendRequests()

                .thenValue([this, promise, start](folly::Unit) {

                    auto end = std::chrono::high_resolution_clock::now();

                    std::chrono::duration<double, milli> duration = end - start;

                    auto lines = results(duration.count());

                    promise->setValue(lines);

                    for (auto& client : clients_)
                        delete client;
                });
        });

    return promise->getFuture();
}
