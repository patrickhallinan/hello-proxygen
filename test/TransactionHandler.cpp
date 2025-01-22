#include "TransactionHandler.h"

#include "HttpClient.h"


void TransactionHandler::setTransaction(proxygen::HTTPTransaction* txn) noexcept {
    txn_ = txn;
}


void TransactionHandler::detachTransaction() noexcept {
    // At this point this could be reused instead of deleted.
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

    // IOBuf is a circular linked list.
    if (inputBuf_) {
        inputBuf_->prependChain(std::move(chain));
    } else {
        inputBuf_ = std::move(chain);
    }
}


void TransactionHandler::onEOM() noexcept {

    std::string body = inputBuf_->moveToFbString().toStdString();
    HttpResponse httpResponse{response_->getStatusCode(), std::move(body)};

    // XXX: If TransactionHandler owned the Promise we could skip the middle-man
    // and send the response directly to the HttpClient user instead of going
    // thru the HttpClient

    // FIXME: onError() can be called after onEOM(). Maybe this should be handled
    // in detachTransaction()? Something needs to be done because we cannot call
    // both httpClient_->requestComplete() and httpClient_->onError()
    httpClient_->requestComplete(std::move(httpResponse));
}


void TransactionHandler::onError(const proxygen::HTTPException& error) noexcept {
    httpClient_->requestError(error);
}

