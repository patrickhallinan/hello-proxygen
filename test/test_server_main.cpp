#include "feature_test.h"
#include "performance_test.h"

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <nlohmann/json.hpp>

using namespace proxygen;


DEFINE_string(test_server_host, "0.0.0.0", "IP address");
DEFINE_int32(test_server_port, 8000, "HTTP port");


template<typename T, typename U>
void setFromConfig(const nlohmann::json& config, const char* key, T* value,
                   U defaultValue) {

    *value = config.contains(key)? config[key].get<T>() : defaultValue;
}


template<typename T, typename U, typename... Args>
void setFromConfig(const nlohmann::json& config, const char* key, T* value,
                   U defaultValue, Args... args) {

    setFromConfig(config, key, value, defaultValue);
    setFromConfig(config, args...);
}


class TestHandler : public RequestHandler {
public:
    TestHandler(folly::EventBase* eb)
        : eb_{eb}
    {}

    void onRequest(std::unique_ptr<HTTPMessage> headers) noexcept override {
    }

    void onBody(std::unique_ptr<folly::IOBuf> chunk) noexcept override {
        if (buf_)
            buf_->prependChain(std::move(chunk));
        else
            buf_ = std::move(chunk);
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
    folly::EventBase* eb_;
    std::unique_ptr<folly::IOBuf> buf_;
    std::string body_;

    const std::string& body() {
        if (buf_)
            body_ = buf_->moveToFbString().toStdString();
        return body_;
    }

    static nlohmann::json config(std::string config) {
        config = folly::trimWhitespace(config);
        return config.empty()? nlohmann::json::parse("{}")
                             : nlohmann::json::parse(config);
    }

    void sendResponse(uint16_t statusCode,
                      const std::string& statusMessage,
                      const std::string& response) {

        ResponseBuilder(downstream_)
            .status(statusCode, statusMessage)
            .header("Content-Type", "text/plain")
            .body(response+"\n")
            .sendWithEOM();
    }
};


class FeatureTestHandler: public TestHandler {
    std::unique_ptr<FeatureTest> ft_;

    struct FeatureTestParams {
        std::string host;
        uint16_t port;
    };
public:
    FeatureTestHandler(folly::EventBase* eb)
        : TestHandler{eb}
    {}

    void onEOM() noexcept override {

        auto params = getTestParams(body());

        if (params.hasError()) {
            auto msg = fmt::format("error parsing json: {}", params.error());
            sendResponse(400, "Bad Request", msg);
        }
        else {
            ft_ = std::make_unique<FeatureTest>(*eb_,
                                                params->host,
                                                params->port);
            ft_->run()
               .thenValue([this](std::string&& result) {
                   nlohmann::json response;
                   response["result"] = result;

                   sendResponse(200, "OK", response.dump());
               });
        }
    }

    using Expected = folly::Expected<FeatureTestParams, std::string>;

    static Expected getTestParams(const std::string& json="{}") {

        try {
            FeatureTestParams p;

            setFromConfig(TestHandler::config(json),
                          "host", &p.host, "localhost",
                          "port", &p.port, 8080);
            return p;
        } catch(const nlohmann::json::exception& e) {
            return folly::makeUnexpected(e.what());
        }
    }
};


class PerformanceTestHandler: public TestHandler {
public:
    PerformanceTestHandler(folly::EventBase* eb)
        : TestHandler{eb}
    {}

    void onEOM() noexcept override {

        auto params = getTestParams(body());

        if (params.hasError()) {
            auto msg = fmt::format("error parsing json: {}", params.error());
            sendResponse(400, "Bad Request", msg);
        }
        else {
            // TODO: make this a handler member
            auto test = std::make_shared<PerformanceTest>(eb_, params.value());

            // capture test to keep it alive
            test->run()
                .thenValue([this, test](std::vector<std::string>&& lines) {

                    sendResponse(200, "OK", folly::join("\n", lines));
                });
        }
    }

    using Expected = folly::Expected<PerformanceTestParams, std::string>;

    static Expected getTestParams(const std::string& json="{}") {

        try {
            PerformanceTestParams p;

            setFromConfig(TestHandler::config(json),
                "host", &p.target_host, "localhost",
                "port", &p.target_port, 8080,
                "num_connections",    &p.number_of_connections, 16,
                "number_of_requests", &p.number_of_requests, 100,
                "payload_size",       &p.payload_size, 2000);

            return p;
        } catch(const nlohmann::json::exception& e) {
            return folly::makeUnexpected(e.what());
        }
    }
};


class UsageHandler: public TestHandler {
    std::string usage_;
public:
    UsageHandler(folly::EventBase* eb)
        : TestHandler(eb)
    {
        auto ftParams = FeatureTestHandler::getTestParams();
        auto perfParams = PerformanceTestHandler::getTestParams();

        usage_ = fmt::format(R"(USAGE:

Request body is JSON (optional)

POST /feature-test
- "host" (string): default {}
- "port" (number): default {}

POST /performance-test
- "host" (string): default {}
- "port" (number): default {}
- "num_connections" (number): default {}
- "number_of_requests" (number): default {}
- "payload_size" (number): default {}

Examples:
curl -X POST http://localhost:8000/feature-test
curl -X POST -d '{{"host":"10.138.1.207"}}' http://localhost:8000/feature-test
)",
        ftParams->host, ftParams->port,

        perfParams->target_host, perfParams->target_port,
        perfParams->number_of_connections, perfParams->number_of_requests,
        perfParams->payload_size);
    }

    void onEOM() noexcept override {
        sendResponse(400, "Bad Request", usage_);
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

        if (httpMessage->getMethodString() == "POST") { // change to enum

            if (httpMessage->getPath() == "/feature-test")
                return new FeatureTestHandler(eb_);

            else if (httpMessage->getPath() == "/performance-test")
                return new PerformanceTestHandler(eb_);
        }

        return new UsageHandler(eb_);
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
    const uint16_t httpPort = FLAGS_test_server_port;
    server.bind({
        {folly::SocketAddress{"0.0.0.0", httpPort, allowNameLookup}, HTTP},
    });

    LOG(INFO) << "Starting HTTP hello test server on port " << httpPort;
    server.start();
    LOG(INFO) << "hellotest server exited";

    return 0;
}
