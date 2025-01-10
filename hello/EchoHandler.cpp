#include "EchoHandler.h"

#include <glog/logging.h>

#include <proxygen/httpserver/ResponseBuilder.h>


void EchoHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> message) noexcept {
    httpMessage_ = std::move(message);
}


void EchoHandler::onBody(std::unique_ptr<folly::IOBuf> chunk) noexcept {
    if (buf_) {
        buf_->prependChain(std::move(chunk));
    } else {
        buf_ = std::move(chunk);
    }
}


void EchoHandler::onEOM() noexcept {
    if (buf_) {
        auto content = buf_->moveToFbString().toStdString();
        LOG(INFO) << "";
        LOG(INFO) << content;

        proxygen::ResponseBuilder(downstream_)
            .status(200, "OK")
            .body(content)
            .sendWithEOM();
    } else {
        proxygen::ResponseBuilder(downstream_)
            .status(400, "Bad Request")
            .body("CONTENT MISSING")
            .sendWithEOM();
    }
}

void EchoHandler::onUpgrade(proxygen::UpgradeProtocol prot) noexcept {
}


void EchoHandler::requestComplete() noexcept {
    delete this;
}


void EchoHandler::onError(proxygen::ProxygenError error) noexcept {
    LOG(ERROR) << proxygen::getErrorString(error);
    delete this;
}
