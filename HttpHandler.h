#pragma once

#include <proxygen/httpserver/RequestHandler.h>


class HttpHandler : public proxygen::RequestHandler {
public:
    HttpHandler(folly::EventBase* eb)
        : eb_{eb}
    {}

    // This will always be the first callback invoked on your handler.
    void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
    void onEOM() noexcept override;

    void onUpgrade(proxygen::UpgradeProtocol prot) noexcept override;

    /**
    * Invoked when request processing has been completed and nothing more
    * needs to be done. This may be a good place to log some stats and
    * clean up resources. This is distinct from onEOM() because it is
    * invoked after the response is fully sent. Once this callback has been
    * received, `downstream_` should be considered invalid.
    */
    void requestComplete() noexcept override;

    /**
    * Request failed. Maybe because of read/write error on socket or client
    * not being able to send request in time.
    *
    * NOTE: Can be invoked at any time (except for before onRequest).
    *
    * No more callbacks will be invoked after this. You should clean up after
    * yourself.
    */
    void onError(proxygen::ProxygenError err) noexcept override;

private:
    folly::EventBase* eb_;
    std::unique_ptr<folly::IOBuf> body_;
    std::unique_ptr<proxygen::HTTPMessage> headers_;
};
