#pragma once

#include <proxygen/lib/http/session/HTTPTransaction.h>


class TransactionHandler : public proxygen::HTTPTransactionHandler {

    class HttpClient* httpClient_;

    // XXX: What to do with this???
    proxygen::HTTPTransaction* txn_{nullptr};

    std::unique_ptr<proxygen::HTTPMessage> response_;
    std::unique_ptr<folly::IOBuf> inputBuf_;
public:
    explicit TransactionHandler(HttpClient* httpClient) : httpClient_{httpClient} {}

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

