#include "Test.h"

#include "HttpClient.h"

#include <folly/io/async/EventBase.h>
#include <gflags/gflags_declare.h>

#include <source_location>

DECLARE_string(hello_host);
DECLARE_int32(hello_port);


static proxygen::HTTPHeaders httpHeaders();


template<typename T, typename U>
void assert_equal(T const& a, U const& b, const std::source_location& loc = std::source_location::current()) {
    if (a != b) {
        std::string msg =  std::format("Failed: '{}' != '{}'  --> {}:{}", a, b, loc.file_name(), loc.line());
        throw std::runtime_error(msg);
    }
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

                              assert_equal(response.body(), "Hello");

                              return httpClient->POST("Echo");
                          })
                          .thenValue([](const HttpResponse&& response) {
                              LOG(INFO) << "Status : " << response.status();
                              LOG(INFO) << "Body   : " << response.body();

                              assert_equal(response.body(), "Echo");
                          })
                          .thenError(folly::tag_t<std::exception>{}, [](const std::exception& e) {
                              LOG(INFO) << e.what();
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

