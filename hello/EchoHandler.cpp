#include "EchoHandler.h"

#include <proxygen/httpserver/ResponseBuilder.h>

#include <gflags/gflags.h>
#include <glog/logging.h>


DEFINE_int32(echo_request_lag, 0, "time in milliseconds to wait before sending response");


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

    auto sendResponse = [this, statusCode, statusMessage, content](){
        proxygen::ResponseBuilder(downstream_)
            .status(statusCode, statusMessage)
            .body(content)
            .sendWithEOM();
    };

    using namespace folly;
    using namespace std;

    if (FLAGS_echo_request_lag > 0) {
        // ensure that Timekeeper is initialized
        static auto tk = folly::detail::getTimekeeperSingleton();

        static auto ms = std::chrono::milliseconds(FLAGS_echo_request_lag);
        static std::atomic<int> count = 0;

        auto p = make_shared<Promise<Unit>>();

        // p must stay alive until continuation is run

        p->getFuture()
            .via(eb_)
            .delayed(ms)
            .thenValue([p, send=std::move(sendResponse)](folly::Unit) {
                send();
            });

        p->setValue(folly::unit);

    } else {
        sendResponse();
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
