#pragma once

#include "TransactionHandler.h"

#include <folly/io/async/EventBase.h>
#include <folly/io/async/SSLContext.h>
#include <folly/futures/Future.h>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/utils/URL.h>

#include <proxygen/lib/utils/WheelTimerInstance.h>


class HttpResponse {
    uint16_t status_;
    std::string body_;
public:
    HttpResponse(uint16_t status, std::string body)
        : status_{status}
        , body_(std::move(body))
    {}

    uint16_t status() const {
        return status_;
    }

    std::string body() const {
        return body_;
    }
};


// TODO: make a simple HttpClient
// move to .cpp
class HttpClient : public proxygen::HTTPConnector::Callback {
    friend class TransactionHandler;

public:
    HttpClient(folly::EventBase*,
               proxygen::WheelTimerInstance&,
               const proxygen::HTTPHeaders&,
               const std::string& url);

    virtual ~HttpClient() = default;

    folly::Future<folly::Unit> connect();
    folly::Future<HttpResponse> GET();
    folly::Future<HttpResponse> POST(const std::string& content);

    // initial SSL related structures
    // rely on gflags
    void initializeSsl(const std::string& caPath,
                       const std::string& nextProtos,
                       const std::string& certPath = "",
                       const std::string& keyPath = "");
    void sslHandshakeFollowup(proxygen::HTTPUpstreamSession* session) noexcept;

protected:
    // proxygen::HTTPConnector::Callback
    void connectSuccess(proxygen::HTTPUpstreamSession*) override;
    void connectError(const folly::AsyncSocketException&) override;

    void requestComplete(HttpResponse) noexcept;
    void requestError(const proxygen::HTTPException&) noexcept;

private:
    proxygen::HTTPMessage headers(proxygen::HTTPMethod, size_t contentLength=0);

    folly::EventBase* eb_{nullptr};
    proxygen::WheelTimerInstance& timer_;
    proxygen::HTTPHeaders headers_;
    proxygen::URL url_;

    proxygen::HTTPUpstreamSession* session_;

    proxygen::HTTPTransaction* txn_{nullptr};

    folly::SSLContextPtr sslContext_;

    bool egressPaused_{false};

    std::unique_ptr<folly::IOBuf> inputBuf_;

    std::unique_ptr<proxygen::HTTPConnector> httpConnector_;

    std::unique_ptr<folly::Promise<folly::Unit>> connectPromise_;
    std::unique_ptr<folly::Promise<HttpResponse>> requestPromise_;
};

