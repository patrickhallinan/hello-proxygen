#include "Test.h"

#include "HttpClient.h"

#include <folly/io/async/EventBase.h>
#include <fmt/format.h>
#include <gflags/gflags_declare.h>

#include <cstdlib>
#include <chrono>
#include <source_location>


DECLARE_string(hello_host);
DECLARE_int32(hello_port);


static proxygen::HTTPHeaders httpHeaders();


#ifdef __cpp_lib_source_location
template<typename T, typename U>
void assert_equal(T const& a, U const& b,
                  const std::source_location& loc = std::source_location::current()) {

    if (a != b) {

        std::string msg =  fmt::format("Failed: '{}' != '{}'  -> {}:{}",
            a, b, loc.file_name(), loc.line());

        throw std::runtime_error(msg);
    }
}
#else

#define  assert_equal(a,b) { \
    if ((a) != (b)) { \
        std::string msg =  fmt::format("Failed: '{}' != '{}'  -> {}:{}", \
            (a), (b), __FILE__, __LINE__); \
        throw std::runtime_error(msg); \
    } \
}

#endif


Test::Test(folly::EventBase& eventBase)
    : eventBase_{eventBase} {

    auto defaultTimeout = std::chrono::milliseconds(5000);

    auto url = fmt::format("http://{}:{}/", FLAGS_hello_host, FLAGS_hello_port);

    httpClient_ = std::make_unique<HttpClient>(&eventBase_,
                                               defaultTimeout,
                                               httpHeaders(),
                                               url);
}


void Test::run() {
    eventBase_.runInEventBaseThread([httpClient=httpClient_.get()]() {

        static bool passed;

        passed = true;

        return httpClient->connect()
                          .thenValue([httpClient](folly::Unit) {
                              LOG(INFO) << "Connected!";

                              return httpClient->GET();
                          })
                          .thenValue([httpClient](const HttpResponse& response) {
                              LOG(INFO) << "Status: " << response.status()
                                  << ", Content: " << response.content();

                              assert_equal(response.status(), 200);
                              assert_equal(response.content(), "Hello");

                              return httpClient->POST("Echo");
                          })
                          .thenValue([httpClient](const HttpResponse& response) {
                              LOG(INFO) << "Status: " << response.status()
                                  << ", Content: " << response.content();

                              assert_equal(response.content(), "Echo");

                              return httpClient->POST("");
                          })
                          .thenValue([](const HttpResponse& response) {
                              LOG(INFO) << "Status: " << response.status()
                                  << ", Content: " << response.content();

                              assert_equal(response.content(), "CONTENT MISSING");
                          })

                          .thenError(folly::tag_t<std::exception>{}, [](const std::exception& e) {
                              passed = false;
                              LOG(INFO) << e.what();
                          })
                          .thenValue([httpClient](folly::Unit) {
                              if (passed) {
                                  LOG(INFO) << "PASSED";
                              }
                              else {
                                  LOG(INFO) << "FAILED";
                              }

                              // XXX: Why does this wait 1m 12s after finishing the test?
                              // HACK: Remove once exit delay is figured out.
                              int status = passed? 0 : 1;
                              exit(status);
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

