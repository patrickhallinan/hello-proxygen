#include "performance_test.h"

#include "HttpClient.h"

#include <fmt/format.h>
#include <folly/futures/ThreadWheelTimekeeper.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <random>

#include <stdexcept>

namespace {

// We really only need atomic<> if was have multiple event bases (i.e. threads)
std::atomic<int> requestCount(0);


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


folly::Future<HttpClient*> send(const PerformanceTestParams& params,
                                HttpClient* client,
                                const std::string& payload) {

    return client->POST("/feature-test", payload)
        .thenValue([&params, client, &payload](const HttpResponse& response) {

            // FIXME: Don't use LOG(FATAL) for response error
            if (response.status() != 200)
                LOG(FATAL) << "received invalid status code from host" << response.status();

            if (response.content() != payload)
                LOG(FATAL) << "invalid response from host. expected size=" << payload.size()
                           << ", actual size=" << response.content().size()
                           << ", content: " << std::endl
                           << response.content() << std::endl;

            if (++requestCount <= params.number_of_requests) {
                return send(params, client, payload);
            }

            return folly::makeFuture(client);
        });
}


folly::Future<folly::Unit> sendRequests(const PerformanceTestParams& params,
                                        folly::EventBase* eventBase,
                                        std::vector<HttpClient*> clients,
                                        const std::string& payload) {

    std::vector<folly::Future<HttpClient*>> clientFutures;


    for (auto* client : clients) {
    	// send() does not block
        clientFutures.push_back(send(params, client, payload));
    }

    return folly::collectAll(std::move(clientFutures))
        .via(eventBase)
        .thenValue([](folly::SemiFuture<std::vector<folly::Try<HttpClient *>>>) {
            return folly::unit;
        });
}



// Originally client->connect() was called on all the clients and folly::collectAll()
// was used to wait for them all to connect but occassionally client->connect() fails
// with a timeout exception.  This work's around the issue by connecting clients one
// by one and doing retries when an exception occurs.
//
// This could be done in batches using folly::collectAll where connection failures
// would got into the next batch.
//
folly::Future<folly::Unit> connect(const PerformanceTestParams* params,
                                   folly::EventBase* eventBase,
                                   std::vector<HttpClient*> clients,
                                   int index=0,
                                   int retries=0) {

    static constexpr int max_retries = 20;

    LOG(INFO) << "connect( " << params << ", " << params->target_host << ":" << params->target_port << " )";

    if (retries >= max_retries) {
        auto msg = fmt::format("Connect to '{}:{}' failed  Exceeded max retries: {}",
            params->target_host, params->target_port, max_retries);
        throw std::runtime_error{msg};
    }

    if (index >= clients.size())
        return folly::unit;

    HttpClient* client = clients[index];

    return client->connect(params->target_host, params->target_port)
        .thenValue([params, eventBase, clients, index, retries](folly::Unit) {
            LOG(INFO) << "CONnect( " << params << ", " << params->target_host << ":" << params->target_port << " )";
            return connect(params, eventBase, std::move(clients), index+1, retries);
        })
        .thenError(folly::tag_t<std::exception>{},
            [params, eventBase, clients=std::move(clients), index, retries]
            (const std::exception& e) mutable {

            LOG(ERROR) << e.what();

            return connect(params, eventBase, std::move(clients), index, retries+1);
        });
}


folly::Future< std::vector<HttpClient*> > getClients(const PerformanceTestParams* params,
                                                     folly::EventBase* eventBase) {
    using namespace std::chrono_literals;

    std::vector<HttpClient*> clients;

    auto endpoint = fmt::format("{}:{}", params->target_host, params->target_port);

    LOG(INFO) << "Connecting to: " << endpoint;

    for (int i = 0 ; i < params->number_of_connections ; i++)
        clients.push_back(new HttpClient(eventBase, 5s, httpHeaders()));

    auto start = std::chrono::high_resolution_clock::now();

    // TODO: Change connect() to return Future<vector<HttpClient*> and stop capturing clients
    return connect(params, eventBase, clients)
        .thenValue([eventBase, clients, start] (folly::Unit) mutable {
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> elapsed = end - start;

            LOG(INFO) << "Connected " << clients.size() << " HttpClient's in "
                << elapsed.count() << " seconds";

            return std::move(clients);
        });
}

std::vector<std::string> results(double total_time_ms,
                                 const PerformanceTestParams& params) {

    double seconds = total_time_ms / 1000.0;
    double averageRequestTime = total_time_ms / params.number_of_requests;
    double requestRate = params.number_of_requests / seconds;
    double mbps = requestRate * 8 * params.payload_size / (1024*10240);

    std::vector<std::string> lines;

#define f fmt::format

    lines.push_back(f("sent {} HTTP POST requests", params.number_of_requests));
    lines.push_back(f("total time: {} seconds", seconds));
    lines.push_back(f("content size: {} bytes", params.payload_size));
    lines.push_back(f("num connections: {} connections", params.number_of_connections));
    lines.push_back(f("avg request Time: {} ms", averageRequestTime));
    lines.push_back(f("request rate: {} requests/second", requestRate));
    lines.push_back(f("bit rate: {} Mbps", mbps));

#undef f

    return lines;
}

} // end namespace anon


PerformanceTest::PerformanceTest(folly::EventBase* eventBase,
                                 const PerformanceTestParams& params)
    : eventBase_{eventBase}
    , params_{params}
    , payload_{makePayload(params.payload_size)}
{}


folly::Future<std::vector<std::string>> PerformanceTest::run() {

    using namespace std;
    using namespace folly;

    auto promise = make_shared< Promise<vector<string>> >();

    getClients(&params_, eventBase_)
        .thenValue([this, promise](vector<HttpClient*> httpClients) {

            auto start = std::chrono::high_resolution_clock::now();

            // XXX: maybe have sendRequests() return Future<vector<HttpClient*>>
            return sendRequests(params_, eventBase_, httpClients, payload_)

                .thenValue([this, promise, start, httpClients](folly::Unit) {

                    auto end = std::chrono::high_resolution_clock::now();

                    std::chrono::duration<double, milli> duration = end - start;

                    auto lines = results(duration.count(), params_);

                    promise->setValue(lines);

                    for (auto& client : httpClients)
                        delete client;
                });
        });

    return promise->getFuture();
}
