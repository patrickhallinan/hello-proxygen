#pragma once

#include <proxygen/lib/http/session/HTTPTransaction.h>


class HttpResponse {
    uint16_t status_;
    std::string content_;
public:
    HttpResponse(uint16_t status, std::string content)
        : status_{status}
        , content_(std::move(content))
    {}

    uint16_t status() const {
        return status_;
    }

    std::string content() const {
        return content_;
    }
};


class TransactionHandler : public proxygen::HTTPTransactionHandler {
    std::unique_ptr<proxygen::HTTPMessage> response_;
    std::unique_ptr<folly::IOBuf> inputBuf_;
    std::unique_ptr<folly::Promise<HttpResponse>> requestPromise_;

public:
    TransactionHandler()
    {}

    folly::Future<HttpResponse> getFuture();

protected:
    void setTransaction(proxygen::HTTPTransaction*) noexcept override;
    void detachTransaction() noexcept override;

    void onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage>) noexcept override;
    void onBody(std::unique_ptr<folly::IOBuf>) noexcept override;
    void onEOM() noexcept override;
    void onError(const proxygen::HTTPException&) noexcept override;

    // NOT USED

    void onTrailers(std::unique_ptr<proxygen::HTTPHeaders>) noexcept override {}
    void onUpgrade(proxygen::UpgradeProtocol) noexcept override {}
    void onEgressResumed() noexcept override {}
    void onEgressPaused() noexcept override {}
};
