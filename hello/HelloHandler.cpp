#include "HelloHandler.h"

#include <glog/logging.h>

#include <proxygen/httpserver/ResponseBuilder.h>


void HelloHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> message) noexcept {
    httpMessage_ = std::move(message);
}


void HelloHandler::onBody(std::unique_ptr<folly::IOBuf> chunk) noexcept {
    if (buf_) {
        buf_->prependChain(std::move(chunk));
    } else {
        buf_ = std::move(chunk);
    }
}


void HelloHandler::onEOM() noexcept {
    if (buf_) {
        content_ = buf_->moveToFbString().toStdString();
        LOG(INFO) << "";
        LOG(INFO) << content_;
    }

    LOG(INFO) << "";

    proxygen::ResponseBuilder(downstream_)
        .status(200, "OK")
        .body("Hello")
        .sendWithEOM();
}

void HelloHandler::onUpgrade(proxygen::UpgradeProtocol prot) noexcept {
}


void HelloHandler::requestComplete() noexcept {
    delete this;
}


void HelloHandler::onError(proxygen::ProxygenError error) noexcept {
    LOG(ERROR) << proxygen::getErrorString(error);
    delete this;
}
