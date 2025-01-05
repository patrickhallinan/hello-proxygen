#include "Test.h"

#include "HttpClient.h"

#include <folly/io/async/EventBase.h>

#include <gflags/gflags_declare.h>


DECLARE_string(hello_host);
DECLARE_int32(hello_port);


static proxygen::HTTPHeaders httpHeaders();


template <typename T>
void assert_equal(T a, T b, std::string msg) {
}


Test::Test(folly::EventBase& eventBase, proxygen::WheelTimerInstance& timer)
    : eventBase_{eventBase}
    , timer_{timer} {

    auto url = fmt::format("http://{}:{}/", FLAGS_hello_host, FLAGS_hello_port);
    httpClient_ = std::make_unique<HttpClient>(&eventBase_, timer_, httpHeaders(), url);
}


void Test::run() {
    HttpClient* http_client = httpClient_.get();

    // XXX: We sometimes crash if this is called after the event loop is started!
    eventBase_.runInEventBaseThread([httpClient=http_client]() {

        return httpClient->connect()
                          .thenValue([httpClient](folly::Unit) {
                              LOG(INFO) << "Connected!";

                              return httpClient->GET();
                          })
                          .thenValue([httpClient](const HttpResponse&& response) {
                              LOG(INFO) << "Status : " << response.status();
                              LOG(INFO) << "Body   : " << response.body();

                              return httpClient->POST("taco");
                          })
                          .thenValue([](const HttpResponse&& response) {
                              LOG(INFO) << "Status : " << response.status();
                              LOG(INFO) << "Body   : " << response.body();
                          })
                          .thenError(folly::tag_t<std::exception>{}, [](const std::exception& e) {
                              LOG(INFO) << "Exception : " << e.what();
                          });
    });
}


proxygen::HTTPHeaders httpHeaders() {
    proxygen::HTTPHeaders headers;

    auto host = fmt::format("{}:{}", FLAGS_hello_host, FLAGS_hello_port);
    headers.add(proxygen::HTTP_HEADER_HOST, host);
    headers.add(proxygen::HTTP_HEADER_USER_AGENT, "test-client");
    headers.add(proxygen::HTTP_HEADER_ACCEPT, "*/*");

    return headers;
}

