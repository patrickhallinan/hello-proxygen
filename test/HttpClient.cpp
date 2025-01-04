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
                       WheelTimerInstance timer,
                       const HTTPHeaders& headers,
                       const std::string& url)
    : eb_{eb}
    , headers_{headers}
    , url_{proxygen::URL{url}} {

        httpConnector_ = std::make_unique<proxygen::HTTPConnector>(this, timer);
    }


folly::Future<folly::Unit> HttpClient::connect() {

    folly::SocketAddress socketAddress{url_.getHost(), url_.getPort(), /*allowNameLookup*/true};

    // intentional static const
    static const folly::SocketOptionMap socketOptions{{{SOL_SOCKET, SO_REUSEADDR}, 1}};

    //httpConnector_->reset();
    httpConnector_->connect(eb_, socketAddress, std::chrono::milliseconds(500), socketOptions);

    connectPromise_.reset(new folly::Promise<folly::Unit>{});
    return connectPromise_->getFuture();
}


folly::Future<HttpResponse> HttpClient::GET() {
    auto transactionHandler = new TransactionHandler{this};
    txn_ = session_->newTransaction(transactionHandler);

    txn_->sendHeaders(headers(proxygen::HTTPMethod::GET));
    txn_->sendEOM();

    requestPromise_.reset(new folly::Promise<HttpResponse>{});
    return requestPromise_->getFuture();
}


folly::Future<HttpResponse> HttpClient::POST(const std::string& content) {
    auto transactionHandler = new TransactionHandler{this};
    txn_ = session_->newTransaction(transactionHandler);

    txn_->sendHeaders(headers(proxygen::HTTPMethod::POST, content.size()));
    txn_->sendBody(folly::IOBuf::copyBuffer(content));
    txn_->sendEOM();

    requestPromise_.reset(new folly::Promise<HttpResponse>{});
    return requestPromise_->getFuture();
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


void HttpClient::connectSuccess(HTTPUpstreamSession* session) {
    if (url_.isSecure()) {
        sslHandshakeFollowup(session);
    }

    session_ = session;

    // Set receive buffer sizes
    session_->setFlowControl(65536, 65536, 65536);

    connectPromise_->setValue(folly::unit);
}


void HttpClient::connectError(const folly::AsyncSocketException& ex) {
    LOG(ERROR) << "Coudln't connect to " << url_.getHostAndPort() << ":" << ex.what();
}


proxygen::HTTPMessage HttpClient::headers(proxygen::HTTPMethod method, size_t contentLength) {
    proxygen::HTTPMessage httpMessage;

    httpMessage.setMethod(method);
    httpMessage.setHTTPVersion(1, 1);
    httpMessage.setURL(url_.makeRelativeURL());
    httpMessage.setSecure(url_.isSecure());

    headers_.forEach([&httpMessage](const string& header, const string& val) {
        httpMessage.getHeaders().add(header, val);
    });

    if (! httpMessage.getHeaders().getNumberOfValues(HTTP_HEADER_USER_AGENT)) {
        httpMessage.getHeaders().add(HTTP_HEADER_USER_AGENT, "http-client");
    }

    if (! httpMessage.getHeaders().getNumberOfValues(HTTP_HEADER_HOST)) {
        httpMessage.getHeaders().add(HTTP_HEADER_HOST, url_.getHostAndPort());
    }

    if (! httpMessage.getHeaders().getNumberOfValues(HTTP_HEADER_ACCEPT)) {
        httpMessage.getHeaders().add(HTTP_HEADER_ACCEPT, "*/*");
    }

    httpMessage.getHeaders().add(HTTP_HEADER_CONTENT_LENGTH, std::to_string(contentLength));

    httpMessage.dumpMessage(4);

    return httpMessage;
}


void HttpClient::requestComplete(HttpResponse httpResponse) noexcept {
    txn_ = nullptr;
    requestPromise_->setValue(std::move(httpResponse));
}


void HttpClient::requestError(const proxygen::HTTPException& error) noexcept {
    txn_ = nullptr;
    LOG(ERROR) << "statusCode: " << error.getHttpStatusCode() << ", errorMessage: " << error.what();
    requestPromise_->setException(error);
}

