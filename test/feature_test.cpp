#include "feature_test.h"

#include "HttpClient.h"

#include <folly/io/async/EventBase.h>
#include <fmt/format.h>

#include <chrono>


static proxygen::HTTPHeaders httpHeaders();


FeatureTest::FeatureTest(folly::EventBase& eventBase, const std::string& host, uint16_t port)
    : eventBase_{eventBase}
    , host_{host}
    , port_{port} {

    auto defaultTimeout = std::chrono::milliseconds(5000);

    httpClient_ = std::make_unique<HttpClient>(&eventBase_,
                                               defaultTimeout,
                                               httpHeaders());
}


folly::Future<std::string> FeatureTest::run() {

    eventBase_.runInEventBaseThread([this]() {

        return httpClient_->connect(host_, port_)
                          .thenValue([this](folly::Unit) {
                              LOG(INFO) << "Connected!";

                              return httpClient_->GET("/feature-test");
                          })
                          .thenValue([this](const HttpResponse& response) {
                              LOG(INFO) << "Status: " << response.status()
                                  << ", Content: " << response.content();

                              assert_equal(response.status(), 200);
                              assert_equal(response.content(), "Hello");

                              return httpClient_->POST("/feature-test", "Echo");
                          })
                          .thenValue([this](const HttpResponse& response) {
                              LOG(INFO) << "Status: " << response.status()
                                  << ", Content: " << response.content();

                              assert_equal(response.content(), "Echo");

                              return httpClient_->POST("/feature-test","");
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

    headers.add(proxygen::HTTP_HEADER_USER_AGENT, "feature-test");
    headers.add(proxygen::HTTP_HEADER_CACHE_CONTROL, "no-store");
    headers.add(proxygen::HTTP_HEADER_ACCEPT, "*/*");

    return headers;
}

