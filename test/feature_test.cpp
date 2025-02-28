#include "feature_test.h"

#include "HttpClient.h"

#include <folly/io/async/EventBase.h>
#include <fmt/format.h>

#include <chrono>


DEFINE_string(hello_host, "127.0.0.1", "IP address");
DEFINE_int32(hello_port, 8080, "HTTP port");


static proxygen::HTTPHeaders httpHeaders();


FeatureTest::FeatureTest(folly::EventBase& eventBase)
    : eventBase_{eventBase} {

    auto defaultTimeout = std::chrono::milliseconds(5000);

    auto url = fmt::format("http://{}:{}/", FLAGS_hello_host, FLAGS_hello_port);

    httpClient_ = std::make_unique<HttpClient>(&eventBase_,
                                               defaultTimeout,
                                               httpHeaders(),
                                               url);
}


folly::Future<std::string> FeatureTest::run() {

    eventBase_.runInEventBaseThread([this]() {

        return httpClient_->connect()
                          .thenValue([this](folly::Unit) {
                              LOG(INFO) << "Connected!";

                              return httpClient_->GET();
                          })
                          .thenValue([this](const HttpResponse& response) {
                              LOG(INFO) << "Status: " << response.status()
                                  << ", Content: " << response.content();

                              assert_equal(response.status(), 200);
                              assert_equal(response.content(), "Hello");

                              return httpClient_->POST("Echo");
                          })
                          .thenValue([this](const HttpResponse& response) {
                              LOG(INFO) << "Status: " << response.status()
                                  << ", Content: " << response.content();

                              assert_equal(response.content(), "Echo");

                              return httpClient_->POST("");
                          })
                          .thenValue([](const HttpResponse& response) {
                              LOG(INFO) << "Status: " << response.status()
                                  << ", Content: " << response.content();

                              assert_equal(response.content(), "CONTENT MISSING");
                          })

                          // TODO: distinguish between test failure and other exception
                          .thenError(folly::tag_t<std::exception>{},
                            [this](const std::exception& e) {

                              passed_ = false;
                              failMsg_ = e.what();
                          })
                          .thenValue([this](folly::Unit) {
                              if (passed_) {
                                  completed_.setValue("PASSED");
                              }
                              else {
                                  auto msg = fmt::format("FAILED: {}", failMsg_);
                                  completed_.setValue(msg);
                              }
                          });
    });

    return completed_.getFuture();
}


proxygen::HTTPHeaders httpHeaders() {
    proxygen::HTTPHeaders headers;

    auto host = fmt::format("{}:{}", FLAGS_hello_host, FLAGS_hello_port);
    headers.add(proxygen::HTTP_HEADER_HOST, host);
    headers.add(proxygen::HTTP_HEADER_USER_AGENT, "feature-test");
    headers.add(proxygen::HTTP_HEADER_CACHE_CONTROL, "no-store");
    headers.add(proxygen::HTTP_HEADER_ACCEPT, "*/*");

    return headers;
}

