#include "Test.h"

#include "HttpClient.h"

#include <folly/futures/ThreadWheelTimekeeper.h>
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>

#include <atomic>
#include <chrono>
#include <cstdlib>


DEFINE_string(target_host, "127.0.0.1", "IP address");
DEFINE_int32(target_port, 8080, "HTTP port");

DEFINE_int32(num_connections, 16, "should be a power of 2");
DEFINE_int32(number_of_requests, 100, "number of http requests");
DEFINE_int32(payload_size, 2000, "number of bytes to send in each request");

void performance_test(folly::EventBase*);

// We really only need atomic<> if was have multiple event bases (i.e. threads)
std::atomic<int> requestCount(0);


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


folly::Future<HttpClient*> send(HttpClient* client, std::string& payload) {
    return client->POST(payload)
        .thenValue([client, &payload](const HttpResponse& response) -> folly::Future<HttpClient*> {

            if (++requestCount <= FLAGS_number_of_requests)
                return send(client, payload);

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


proxygen::HTTPHeaders httpHeaders() {
    proxygen::HTTPHeaders headers;

    auto host = fmt::format("{}:{}", FLAGS_target_host, FLAGS_target_port);
    headers.add(proxygen::HTTP_HEADER_HOST, host);
    headers.add(proxygen::HTTP_HEADER_USER_AGENT, "test-client");
    headers.add(proxygen::HTTP_HEADER_ACCEPT, "*/*");

    return headers;
}


std::string makePayload(int size) {
    std::string payload;
    for (int i = 0 ; i < FLAGS_payload_size ; i++)
        payload.push_back('a');
    return payload;
}


folly::Future< std::vector<HttpClient*> > getClients(folly::EventBase* eventBase, int count) {
    using namespace std::chrono_literals;

    std::vector<HttpClient*> clients;
    std::vector<folly::Future<folly::Unit>> connectFutures;

    auto url = fmt::format("http://{}:{}/", FLAGS_target_host, FLAGS_target_port);

    for (int i = 0 ; i < count ; i++) {

        clients.push_back(new HttpClient(eventBase, 5s, httpHeaders(), url));

        connectFutures.push_back(clients[i]->connect());
    }

    return folly::collectAll(std::move(connectFutures))
        .via(eventBase)
        .thenValue([eventBase, clients=std::move(clients)]
                   (std::vector<folly::Try<folly::Unit>> results) mutable {

            for (const auto& result : results) {
                if (! result.hasValue()) {
                    LOG(ERROR) << "Exception: " << result.exception().what();
                    exit(1);
                }
            }

            LOG(INFO) << "All HttpClient's connected";

            return std::move(clients);
        });
}


void performance_test(folly::EventBase* eventBase) {

    static std::vector<HttpClient*> clients;
    static std::string payload;
    static std::chrono::time_point<std::chrono::high_resolution_clock> start;

    getClients(eventBase, FLAGS_num_connections)
        .thenValue([eventBase](std::vector<HttpClient*> httpClients) {

            clients = httpClients;
            payload = makePayload(FLAGS_payload_size);
            start = std::chrono::high_resolution_clock::now();

            return sendRequests(eventBase, clients, payload);
        })
        .thenValue([](folly::Unit) {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;

            const long ONE_MEG = 0x100000;
            double seconds = duration.count() / 1000.0;
            double requestRate = FLAGS_number_of_requests / seconds;
            double averageRequestTime = duration.count() / FLAGS_number_of_requests;
            double bitRate = 8 * FLAGS_payload_size * FLAGS_number_of_requests / seconds
                             / ONE_MEG;


            LOG(INFO) << "";
            LOG(INFO) << "took " << duration.count() / 1000.0 << " seconds"
                      << " to send " << FLAGS_number_of_requests
                      << " with " << clients.size() << " connections";

            LOG(INFO) << "request rate     : " << requestRate << " requests/second";
            LOG(INFO) << "avg request Time : " << averageRequestTime << " ms";
            LOG(INFO) << "bit rate         : " << bitRate << " Mbps";

            for (auto& client : clients)
                delete client;

            // HACK: Figure out how to exit immediately after this ends and remove exit() call
            exit(0);
        });
}
