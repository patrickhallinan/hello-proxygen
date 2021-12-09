#include "HttpHandler.h"

#include <glog/logging.h>

#include <proxygen/httpserver/ResponseBuilder.h>


void HttpHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept {
    headers_ = std::move(headers);

    LOG(INFO) << "path=" << headers_->getPath();
}


void HttpHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {
    if (body_) {
        body_->prependChain(std::move(body));
    } else {
        body_ = std::move(body);
    }
}


void HttpHandler::onEOM() noexcept {
    proxygen::ResponseBuilder(downstream_)
        .status(200, "OK")
        .body("Hello\n")
        .sendWithEOM();
}

void HttpHandler::onUpgrade(proxygen::UpgradeProtocol prot) noexcept {
}


void HttpHandler::requestComplete() noexcept {
    delete this;
}


void HttpHandler::onError(proxygen::ProxygenError error) noexcept {
    LOG(ERROR) << proxygen::getErrorString(error);
    delete this;
}
