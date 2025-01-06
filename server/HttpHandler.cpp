#include "HttpHandler.h"

#include <glog/logging.h>

#include <proxygen/httpserver/ResponseBuilder.h>


void HttpHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> message) noexcept {
    httpMessage_ = std::move(message);

    LOG(INFO) << httpMessage_->getMethodString()
        << " " << httpMessage_->getPath()
        << " HTTP/" << httpMessage_->getProtocolString();

    httpMessage_->getHeaders().forEach([](const std::string& header,
                                          const std::string& val) {
        LOG(INFO) << header << ": " << val;
    });
}


void HttpHandler::onBody(std::unique_ptr<folly::IOBuf> chunk) noexcept {
    if (buf_) {
        buf_->prependChain(std::move(chunk));
    } else {
        buf_ = std::move(chunk);
    }
}


void HttpHandler::onEOM() noexcept {
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

void HttpHandler::onUpgrade(proxygen::UpgradeProtocol prot) noexcept {
}


void HttpHandler::requestComplete() noexcept {
    delete this;
}


void HttpHandler::onError(proxygen::ProxygenError error) noexcept {
    LOG(ERROR) << proxygen::getErrorString(error);
    delete this;
}
