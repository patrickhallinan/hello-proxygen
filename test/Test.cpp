#include "Test.h"

#include "HttpClient.h"

#include <folly/io/async/EventBase.h>
//#include <proxygen/lib/http/HTTPCommonHeaders.h>

#include <gflags/gflags_declare.h>


DECLARE_string(hello_host);
DECLARE_int32(hello_port);


proxygen::HTTPHeaders httpHeaders();


Test::Test(folly::EventBase& eventBase, proxygen::WheelTimerInstance& timer) {
    //eventBase.runInEventBaseThreadAndWait([&]() {
    eventBase.runInEventBaseThread([&]() {
        std::cout << "runInEventBaseThreadAndWait()\n";

        auto url = fmt::format("http://{}:{}/", FLAGS_hello_host, FLAGS_hello_port);

        httpClient_ = std::make_unique<HttpClient>(&eventBase, timer, url, httpHeaders());

        return httpClient_->POST("nacho")
              .thenValue([](const HttpResponse&& response) {
                  LOG(INFO) << "Status : " << response.status();
                  LOG(INFO) << "Body   : " << response.body();
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

