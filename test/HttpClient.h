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
               proxygen::WheelTimerInstance,
               const std::string& url,
               const proxygen::HTTPHeaders&);

    virtual ~HttpClient() = default;

    folly::Future<HttpResponse> POST(const std::string& content);

    static proxygen::HTTPHeaders parseHeaders(const std::string& headersString);

    // initial SSL related structures
    // rely on gflags
    void initializeSsl(const std::string& caPath,
                       const std::string& nextProtos,
                       const std::string& certPath = "",
                       const std::string& keyPath = "");
    void sslHandshakeFollowup(proxygen::HTTPUpstreamSession* session) noexcept;

    // HTTPConnector methods
    void connectSuccess(proxygen::HTTPUpstreamSession* session) override;
    void connectError(const folly::AsyncSocketException& ex) override;

    void sendRequest(proxygen::HTTPTransaction* txn);

    // Getters

    folly::SSLContextPtr getSSLContext() {
        return sslContext_;
    }

    const std::string& getServerName() const;

    void setFlowControlSettings(int32_t recvWindow);

protected:
    void setupHeaders();
    void requestComplete(HttpResponse) noexcept;
    void requestError(const proxygen::HTTPException&) noexcept;

    folly::EventBase* eb_{nullptr};

    proxygen::HTTPTransaction* txn_{nullptr};
    proxygen::HTTPMethod httpMethod_;
    proxygen::URL url_;
    proxygen::HTTPMessage request_;

    folly::SSLContextPtr sslContext_;

    int32_t recvWindow_{65536};
    bool egressPaused_{false};

    std::string requestBody_;

    std::unique_ptr<folly::IOBuf> inputBuf_;

    //folly::HHWheelTimer::UniquePtr eventTimer_;
    std::unique_ptr<proxygen::HTTPConnector> httpConnector_;

    std::unique_ptr<folly::Promise<HttpResponse>> promise_;
};

