#include "feature_test.h"
#include "performance_test.h"

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>


using namespace proxygen;


class TestHandler : public RequestHandler {
    folly::EventBase* eb_;
    FeatureTest ft_;

public:
    TestHandler(folly::EventBase* eb)
        : eb_{eb}
        , ft_{*eb}
    {}

    void onRequest(std::unique_ptr<HTTPMessage> headers) noexcept override {
    }

    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {
    }

    void onEOM() noexcept override {

        ft_.run()
           .thenValue([this](std::string&& result) {
               sendResponse(result);
           });
    }

    void onUpgrade(UpgradeProtocol protocol) noexcept override {
    }

    void requestComplete() noexcept override {
        delete this;
    }

    void onError(ProxygenError err) noexcept override {
        delete this;
    }
private:
    void sendResponse(const std::string& response) {
        ResponseBuilder(downstream_)
            .status(200, "OK")
            .header("Content-Type", "text/plain")
            .body(response)
            .sendWithEOM();
    }
};


class TestHandlerFactory : public RequestHandlerFactory {
    thread_local static folly::EventBase* eb_;
public:
    void onServerStart(folly::EventBase* eb) noexcept override {
        eb_ = eb;
        LOG(INFO) << "TestHandlerFactory::onServerStart()";
    }

    void onServerStop() noexcept override {
        eb_ = nullptr;
        LOG(INFO) << "TestHandlerFactory::onServerStop()";
    }

    RequestHandler* onRequest(RequestHandler*, HTTPMessage*) noexcept override {
        return new TestHandler(eb_);
    }
};


thread_local folly::EventBase* TestHandlerFactory::eb_{nullptr};


int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    const unsigned cores = std::thread::hardware_concurrency();

    HTTPServerOptions options;
    options.threads = cores? cores : 2;
    options.idleTimeout = std::chrono::milliseconds(60'000);
    options.shutdownOn = {SIGINT, SIGTERM};
    options.handlerFactories = RequestHandlerChain()
        .addThen<TestHandlerFactory>()
        .build();

    HTTPServer server(std::move(options));

    constexpr auto allowNameLookup = true;
    constexpr auto HTTP = proxygen::HTTPServer::Protocol::HTTP;
    constexpr uint16_t httpPort = 8000;
    server.bind({
        {folly::SocketAddress{"0.0.0.0", httpPort, allowNameLookup}, HTTP},
    });

    LOG(INFO) << "Starting HTTP test server on port " << httpPort;
    server.start();
    LOG(INFO) << "HTTP server exited";

    return 0;
}
