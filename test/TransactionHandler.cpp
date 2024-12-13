#include "TransactionHandler.h"

#include "HttpClient.h"


void TransactionHandler::setTransaction(proxygen::HTTPTransaction* txn) noexcept {
    txn_ = txn;
}


void TransactionHandler::detachTransaction() noexcept {
    delete this;
}


void TransactionHandler::onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
    response_ = std::move(msg);
    //response_.dumpMessage(4);
}


void TransactionHandler::onBody(std::unique_ptr<folly::IOBuf> chain) noexcept {
    if (!chain) {
        return;
    }

    if (inputBuf_) {
        inputBuf_->prependChain(std::move(chain));
    } else {
        inputBuf_ = std::move(chain);
    }
}


void TransactionHandler::onEOM() noexcept {

    std::string body = inputBuf_->moveToFbString().toStdString();
    HttpResponse httpResponse{response_->getStatusCode(), std::move(body)};

    httpClient_->requestComplete(std::move(httpResponse));
}


void TransactionHandler::onError(const proxygen::HTTPException& error) noexcept {
    httpClient_->requestError(error);
}

