#include "feature_test.h"
#include "performance_test.h"

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <nlohmann/json.hpp>

using namespace proxygen;


// Feature Test
DECLARE_string(hello_host);
DECLARE_int32(hello_port);

// Performance Test
DECLARE_string(target_host);
DECLARE_int32(target_port);

DECLARE_int32(num_connections);
DECLARE_int32(number_of_requests);
DECLARE_int32(payload_size);
DECLARE_bool(validate_content);


void trySet(const nlohmann::json& config, const char* key) {
    if (config.contains(key)) {
        std::string value = config[key].dump();
        gflags::SetCommandLineOption(key, value.c_str());
    }
}


std::string defaultFlagValue(const char* flagName) {
    gflags::CommandLineFlagInfo info;
    if (gflags::GetCommandLineFlagInfo(flagName, &info)) {
        return info.default_value;
    }
    else {
        return fmt::format("'{}' gflag not found", flagName);
    }
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

    using Expected = folly::Expected<folly::Unit,std::string>;

    Expected trySetFlags(std::vector<const char*> flags) {

        auto json = folly::trimWhitespace(body());

        if (! json.empty()) {
            try {
                auto config = nlohmann::json::parse(json.toString());

                for (const char* flagName : flags) {
                    trySet(config, flagName);
                }
            } catch(const nlohmann::json::exception& e) {
                return folly::makeUnexpected(e.what());
            }
        }

        return folly::unit;
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

public:
    FeatureTestHandler(folly::EventBase* eb)
        : TestHandler{eb}
    {}

    void onEOM() noexcept override {
        // FIXME: I am broken!  setting gflags breaks with concurrent requests.
        // It also changes default values.  Shouldn't use gflags::FlagSaver
        // to save and restore state because that adds brokenness to brokenness.
        // There needs to be a way to override gflag values so that a concurrent
        // request has it's own separate state. Same with PerformanceTestHandler

        auto result = trySetFlags({"hello_host", "hello_port"});

        if (result.hasError()) {
            auto msg = fmt::format("error parsing json: {}", result.error());
            sendResponse(400, "Bad Request", msg);
        }
        else {
            ft_ = std::make_unique<FeatureTest>(*eb_);

            ft_->run()
               .thenValue([this](std::string&& result) {
                   nlohmann::json response;
                   response["result"] = result;

                   sendResponse(200, "OK", response.dump());
               });
        }
    }
};


class PerformanceTestHandler: public TestHandler {
public:
    PerformanceTestHandler(folly::EventBase* eb)
        : TestHandler{eb}
    {}

    void onEOM() noexcept override {

        auto result = trySetFlags({"target_host", "target_port", "num_connections",
                    "number_of_requests", "payload_size", "validate_content"});

        if (result.hasError()) {
            auto msg = fmt::format("error parsing json: {}", result.error());
            sendResponse(400, "Bad Request", msg);
        }
        else {
            performance_test(eb_)
               .thenValue([this](std::vector<std::string>&& lines) {

                   sendResponse(200, "OK", folly::join("\n", lines));
               });
        }
    }
};


class UsageHandler: public TestHandler {
    std::string usage_;
public:
    UsageHandler(folly::EventBase* eb)
        : TestHandler(eb)
    {
        usage_ = fmt::format(R"nacho(USAGE:
POST /feature-test
Request body is optional JSON
- "hello_host" (string): default {}
- "hello_port" (number): default {}

POST /performance-test
- "target_host" (string): default {}
- "target_port" (number): default {}
- "num_connections" (number): default {}
- "number_of_requests" (number): default {}
- "payload_size" (number): default {}
- "validate_content" (number): default {}

Examples:
curl -X POST http://localhost:8000/feature-test
curl -X POST -d '{{"hello_host":"10.138.1.207"}}' http://localhost:8000/feature-test
)nacho",
        defaultFlagValue("hello_host"), defaultFlagValue("hello_port"),

        defaultFlagValue("target_host"), defaultFlagValue("target_port"),
        defaultFlagValue("num_connections"), defaultFlagValue("number_of_requests"),
        defaultFlagValue("payload_size"), defaultFlagValue("validate_content")
        );
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
    constexpr uint16_t httpPort = 8000;
    server.bind({
        {folly::SocketAddress{"0.0.0.0", httpPort, allowNameLookup}, HTTP},
    });

    LOG(INFO) << "Starting HTTP test server on port " << httpPort;
    server.start();
    LOG(INFO) << "HTTP server exited";

    return 0;
}
