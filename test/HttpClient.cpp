#include "HttpClient.h"

#include "TransactionHandler.h"

#include <iostream>
#include <sys/stat.h>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/io/async/SSLContext.h>
#include <folly/io/async/SSLOptions.h>
#include <folly/portability/GFlags.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/codec/HTTP2Codec.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>

#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/HTTPMethod.h>

using namespace folly;
using namespace proxygen;
using namespace std;

DECLARE_int32(recv_window);


HttpClient::HttpClient(EventBase* eb,
                       const std::string& url,
                       const HTTPHeaders& headers)
    : eb_{eb}
    , url_{proxygen::URL{url}} {

    headers.forEach([this](const string& header, const string& val) {
        request_.getHeaders().add(header, val);
    });

    // Scheduled events and timeout callbacks are registered with the timer.  If an event
    // occurs before a timeout then the timeout callback is removed from the timer. There
    // can be no race condition since timeout events and all other events are serviced
    // from the same thread.
    //
    // There can be only one!  This is a big object.
    eventTimer_ = folly::HHWheelTimer::newTimer(
        eb_,
        std::chrono::milliseconds(folly::HHWheelTimer::DEFAULT_TICK_INTERVAL),
        folly::AsyncTimeout::InternalEnum::NORMAL,
        std::chrono::milliseconds(5000));

    httpConnector_ = std::make_unique<proxygen::HTTPConnector>(this, eventTimer_.get());
}


folly::Future<HttpResponse> HttpClient::post(const std::string& content) {
    requestBody_ = content;
    httpMethod_ = *proxygen::stringToMethod("POST");

    folly::SocketAddress socketAddress{url_.getHost(), url_.getPort(), /*allowNameLookup*/true};

    // For some reason the connect methods takes a const ref
    static const folly::SocketOptionMap socketOptions{{{SOL_SOCKET, SO_REUSEADDR}, 1}};

    httpConnector_->reset();
    httpConnector_->connect(eb_, socketAddress, std::chrono::milliseconds(500), socketOptions);

    promise_.reset(new folly::Promise<HttpResponse>{});
    return promise_->getFuture();
}


void HttpClient::initializeSsl(const string& caPath,
                               const string& nextProtos,
                               const string& certPath,
                               const string& keyPath) {

    sslContext_ = std::make_shared<folly::SSLContext>();
    sslContext_->setOptions(SSL_OP_NO_COMPRESSION);
    sslContext_->setCipherList(folly::ssl::SSLCommonOptions::ciphers());

    if (!caPath.empty()) {
        sslContext_->loadTrustedCertificates(caPath.c_str());
    }

    if (!certPath.empty() && !keyPath.empty()) {
        sslContext_->loadCertKeyPairFromFiles(certPath.c_str(), keyPath.c_str());
    }

    list<string> nextProtoList;
    folly::splitTo<string>(
      ',', nextProtos, std::inserter(nextProtoList, nextProtoList.begin()));
    sslContext_->setAdvertisedNextProtocols(nextProtoList);
}


void HttpClient::sslHandshakeFollowup(HTTPUpstreamSession* session) noexcept {
  AsyncSSLSocket* sslSocket =
      dynamic_cast<AsyncSSLSocket*>(session->getTransport());

  const unsigned char* nextProto = nullptr;
  unsigned nextProtoLength = 0;
  sslSocket->getSelectedNextProtocol(&nextProto, &nextProtoLength);
  if (nextProto) {
    VLOG(1) << "Client selected next protocol "
            << string((const char*)nextProto, nextProtoLength);
  } else {
    VLOG(1) << "Client did not select a next protocol";
  }

  // Note: This ssl session can be used by defining a member and setting
  // something like sslSession_ = sslSocket->getSSLSession() and then
  // passing it to the connector::connectSSL() method
}


void HttpClient::setFlowControlSettings(int32_t recvWindow) {
    recvWindow_ = recvWindow;
}


void HttpClient::connectSuccess(HTTPUpstreamSession* session) {
    if (url_.isSecure()) {
        sslHandshakeFollowup(session);
    }

    session->setFlowControl(recvWindow_, recvWindow_, recvWindow_);
    auto transactionHandler = new TransactionHandler{this};
    txn_ = session->newTransaction(transactionHandler);
    setupHeaders();
    txn_->sendHeaders(request_);

    if (httpMethod_ == HTTPMethod::POST) {
        // TODO: don't copy the string twice.
        txn_->sendBody(folly::IOBuf::copyBuffer(requestBody_));
    }

    txn_->sendEOM();

    session->closeWhenIdle();
}


void HttpClient::connectError(const folly::AsyncSocketException& ex) {
    LOG(ERROR) << "Coudln't connect to " << url_.getHostAndPort() << ":" << ex.what();
}


void HttpClient::setupHeaders() {
    request_.setMethod(httpMethod_);
    request_.setHTTPVersion(1, 1);
    request_.setURL(url_.makeRelativeURL());
    request_.setSecure(url_.isSecure());

    auto& headers = request_.getHeaders();

    /*
    if (! headers.getNumberOfValues(HTTP_HEADER_USER_AGENT)) {
        headers.add(HTTP_HEADER_USER_AGENT, "hello-agent");
    }
    */

    if (! headers.getNumberOfValues(HTTP_HEADER_HOST)) {
        headers.add(HTTP_HEADER_HOST, url_.getHostAndPort());
    }

    if (! headers.getNumberOfValues(HTTP_HEADER_ACCEPT)) {
        headers.add("Accept", "*/*");
    }

    request_.dumpMessage(4);
}


void HttpClient::requestComplete(HttpResponse httpResponse) noexcept {
    promise_->setValue(std::move(httpResponse));
}


void HttpClient::requestError(const proxygen::HTTPException& error) noexcept {
    LOG(ERROR) << "statusCode: " << error.getHttpStatusCode() << ", errorMessage: " << error.what();
    promise_->setException(error);
}


const string& HttpClient::getServerName() const {

    const string& res = request_.getHeaders().getSingleOrEmpty(HTTP_HEADER_HOST);

    if (res.empty()) {
        return url_.getHost();
    }

    return res;
}

