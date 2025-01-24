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
    int statusCode;
    std::string statusMessage;
    std::string content;

    if (buf_) {
        statusCode = 200;
        statusMessage = "OK";
        content = buf_->moveToFbString().toStdString();

        LOG(INFO) << "";
        LOG(INFO) << content;
    } else {
        statusCode = 400;
        statusMessage = "Bad Request";
        content = "CONTENT MISSING";
    }

    proxygen::ResponseBuilder(downstream_)
        .status(statusCode, statusMessage)
        .body(content)
        .sendWithEOM();
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
