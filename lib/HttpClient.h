#pragma once

#include "TransactionHandler.h"

#include <folly/io/async/EventBase.h>
#include <folly/io/async/SSLContext.h>
#include <folly/futures/Future.h>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/utils/URL.h>

#include <chrono>


// TODO: make a simpler HttpClient and move this to .cpp
class HttpClient : public proxygen::HTTPConnector::Callback {
    friend class TransactionHandler;

public:
    // TODO: Replace url with host/port and put path in GET()/POST()
    HttpClient(folly::EventBase*,
               std::chrono::milliseconds timeout,
               const proxygen::HTTPHeaders&,
               const std::string& url);

    virtual ~HttpClient() = default;

    folly::Future<folly::Unit> connect();

    folly::Future<HttpResponse> GET();
    folly::Future<HttpResponse> POST(const std::string& content);

    // TODO: Implement https

protected:
    // proxygen::HTTPConnector::Callback
    void connectSuccess(proxygen::HTTPUpstreamSession*) override;
    void connectError(const folly::AsyncSocketException&) override;

private:
    proxygen::HTTPMessage createHttpMessage(proxygen::HTTPMethod, size_t contentLength=0);

    folly::EventBase* eb_{nullptr};
    proxygen::HTTPHeaders headers_;
    proxygen::URL url_;

    proxygen::HTTPUpstreamSession* session_;

    std::unique_ptr<folly::IOBuf> inputBuf_;

    std::unique_ptr<proxygen::HTTPConnector> httpConnector_;

    std::unique_ptr<folly::Promise<folly::Unit>> connectPromise_;

    // for metrics
    std::chrono::time_point<std::chrono::high_resolution_clock> connectStartTime_;
};

