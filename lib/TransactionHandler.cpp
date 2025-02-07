#include "TransactionHandler.h"

#include "HttpClient.h"


folly::Future<HttpResponse> TransactionHandler::getFuture() {
    if (requestPromise_) {
        throw std::runtime_error("Cannot call TransactionHandler::getFuture() twice");
    }

    requestPromise_ = std::make_unique<folly::Promise<HttpResponse>>();
    return requestPromise_->getFuture();
}


void TransactionHandler::setTransaction(proxygen::HTTPTransaction* txn) noexcept {
    // first event. txn is good until detachTransaction()
}


void TransactionHandler::detachTransaction() noexcept {
    // requestPromise_ will be reset if onError() is called
    if (requestPromise_) {
        std::string body = inputBuf_->moveToFbString().toStdString();
        HttpResponse httpResponse{response_->getStatusCode(), std::move(body)};

        requestPromise_->setValue(std::move(httpResponse));
        requestPromise_.reset();
    }

    // this could be reused instead of deleted.
    delete this;
}


void TransactionHandler::onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
    response_ = std::move(msg);
    //response_.dumpMessage(4);
}


void TransactionHandler::onBody(std::unique_ptr<folly::IOBuf> link) noexcept {
    if (!link) {
        return;
    }

    // IOBuf is a circular linked list.
    if (inputBuf_) {
        inputBuf_->prependChain(std::move(link));
    } else {
        inputBuf_ = std::move(link);
    }
}


void TransactionHandler::onEOM() noexcept {
    // onError() can be called after onEOM().  I'm not sure why but maybe
    // onTrailers() could be called after this.  Whatever the reason we
    // cannot set the request promise here safely.
}


void TransactionHandler::onError(const proxygen::HTTPException& error) noexcept {
    requestPromise_->setException(error);
    requestPromise_.reset();
}

