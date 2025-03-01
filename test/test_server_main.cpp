#include "feature_test.h"
#include "performance_test.h"

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <nlohmann/json.hpp>

using namespace proxygen;


class TestHandler : public RequestHandler {
    folly::EventBase* eb_;
    std::unique_ptr<folly::IOBuf> buf_;

public:
    TestHandler(folly::EventBase* eb)
        : eb_{eb}
    {}

    void onRequest(std::unique_ptr<HTTPMessage> headers) noexcept override {
    }

    void onBody(std::unique_ptr<folly::IOBuf> chunk) noexcept override {
        if (buf_) {
            buf_->prependChain(std::move(chunk));
        } else {
            buf_ = std::move(chunk);
        }
    }

    //void onEOM() noexcept override {}

    void onUpgrade(UpgradeProtocol protocol) noexcept override {
    }

    void requestComplete() noexcept override {
        delete this;
    }

    void onError(ProxygenError err) noexcept override {
        delete this;
    }
protected:
    std::string body() {
        return buf_->moveToFbString().toStdString();
    }

    void sendResponse(uint16_t statusCode,
                      const std::string& statusMessage,
                      const std::string& response) {

        ResponseBuilder(downstream_)
            .status(statusCode, statusMessage)
            .header("Content-Type", "text/plain")
            .body(response)
            .sendWithEOM();
    }
};


class FeatureTestHandler : public TestHandler {
    FeatureTest ft_;

public:
    FeatureTestHandler(folly::EventBase* eb)
        : TestHandler{eb}
        , ft_{*eb}
    {}

    void onEOM() noexcept override {

        ft_.run()
           .thenValue([this](std::string&& result) {
               nlohmann::json response;
               response["result"] = result;

               sendResponse(200, "OK", response.dump());
           });

    }
};


class UsageHandler: public TestHandler {
public:
    UsageHandler(folly::EventBase* eb)
        : TestHandler(eb)
    {}

    void onEOM() noexcept override {
        sendResponse(400, "Bad Request", usage());
    }

private:
    std::string usage() {
        return R"(USAGE:

POST /feature-test
Request body is the HTTP endpoint, like: http://locahost:8080

POST /performance-test
Request body is the HTTP endpoint, like: http://locahost:8080

example:  curl -X POST -d "http://localhost:8080" http://localhost:8000/feature-test
)";
    }
};


class TestHandlerFactory : public RequestHandlerFactory {
    thread_local static folly::EventBase* eb_;
public:
    void onServerStart(folly::EventBase* eb) noexcept override {
        eb_ = eb;
    }

    void onServerStop() noexcept override {
        eb_ = nullptr;
    }

    RequestHandler* onRequest(RequestHandler*, HTTPMessage* httpMessage) noexcept override {
        if (httpMessage->getMethodString() == "POST" && // change to enum
            httpMessage->getPath() == "/feature-test") {

            return new FeatureTestHandler(eb_);
        } else {
            return new UsageHandler(eb_);
        }
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
