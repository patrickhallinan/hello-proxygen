#include "Test.h"

#include "HttpClient.h"

#include <folly/futures/ThreadWheelTimekeeper.h>
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <random>


DEFINE_string(target_host, "127.0.0.1", "IP address");
DEFINE_int32(target_port, 8080, "HTTP port");

DEFINE_int32(num_connections, 16, "should be a power of 2");
DEFINE_int32(number_of_requests, 100, "number of http requests");
DEFINE_int32(payload_size, 2000, "number of bytes to send in each request");
DEFINE_bool(validate_content, false, "verify response content equals request content");


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

            if (response.status() != 200)
                LOG(FATAL) << "received invalid status code from host" << response.status();

            // I don't think there would be any value added to log the random payload
            if (FLAGS_validate_content && response.content() != payload)
                LOG(FATAL) << "invalid response from host. expected size=" << payload.size()
                           << ", actual size=" << response.content().size()
                           << ", content: " << std::endl
                           << response.content() << std::endl;

            if (requestCount++ < FLAGS_number_of_requests)
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

    static const std::string characters{ "abcdefghijklmnopqrstuvwxyz"
                                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "0123456789" };

    std::mt19937 random_number_generator{ std::random_device{}() };
    std::uniform_int_distribution<int> distribution(0, characters.size() - 1);

    for (int i = 0 ; i < size ; i++)
        payload += characters[distribution(random_number_generator)];

    return payload;
}


folly::Future< std::vector<HttpClient*> > getClients(folly::EventBase* eventBase, int count) {
    using namespace std::chrono_literals;

    std::vector<HttpClient*> clients;
    std::vector<folly::Future<folly::Unit>> connectFutures;


    auto url = fmt::format("http://{}:{}/", FLAGS_target_host, FLAGS_target_port);

    LOG(INFO) << "Connecting to: " << url;

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

            double seconds = duration.count() / 1000.0;
            double averageRequestTime = duration.count() / FLAGS_number_of_requests;
            double requestRate = FLAGS_number_of_requests / seconds;
            double mbps = requestRate * 8 * FLAGS_payload_size / (1024*10240);

            LOG(INFO) << "";
            LOG(INFO) << "sent " << FLAGS_number_of_requests << " HTTP POST requests";
            LOG(INFO) << "total time       : " << seconds << " seconds";

            LOG(INFO) << "content size     : " << FLAGS_payload_size << " bytes";
            LOG(INFO) << "num connections  : " << clients.size() << " connections";

            LOG(INFO) << "avg request Time : " << averageRequestTime << " ms";
            LOG(INFO) << "request rate     : " << requestRate << " requests/second";
            LOG(INFO) << "bit rate         : " << mbps << " Mbps";

            for (auto& client : clients)
                delete client;

            // HACK: Figure out how to exit immediately after this ends and remove exit() call
            exit(0);
        });
}
