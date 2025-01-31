#include "HttpClient.h"

#include "TransactionHandler.h"

#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>

#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/HTTPMethod.h>

using namespace folly;
using namespace proxygen;
using namespace std;


DEFINE_bool(log_connect_time, false, "Log number of milliseconds to connect to endpoint");
DEFINE_int32(connect_timeout, 500, "HttpClient connect Timout in milliseconds");


HttpClient::HttpClient(EventBase* eb,
                       std::chrono::milliseconds defaultTimeout,
                       const HTTPHeaders& headers,
                       const std::string& url)
    : eb_{eb}
    , headers_{headers}
    , url_{proxygen::URL{url}} {

    // uses HHWheelTimer from EventBase
    proxygen::WheelTimerInstance timer{defaultTimeout, eb};

    httpConnector_ = std::make_unique<proxygen::HTTPConnector>(this, timer);
}


folly::Future<folly::Unit> HttpClient::connect() {
    connectStartTime_ = std::chrono::high_resolution_clock::now();

    folly::SocketAddress socketAddress{url_.getHost(), url_.getPort(), /*allowNameLookup*/true};

    static const folly::SocketOptionMap socketOptions{{{SOL_SOCKET, SO_REUSEADDR}, 1}};

    // Must initialize connectPromise before HttpClient::connect() because
    // HttpClient::connect() can fail and call HttpClient::connectError()
    // before http_Connector_->connect() finishes.  For example, this will
    // happen if HTTPConnector fails to create a socket.
    connectPromise_.reset(new folly::Promise<folly::Unit>{});

    // reset() ensures HTTPConnector is ready for subsequent connections.
    httpConnector_->reset();
    httpConnector_->connect(eb_,
                            socketAddress,
                            std::chrono::milliseconds(FLAGS_connect_timeout),
                            socketOptions);

    return connectPromise_->getFuture();
}


folly::Future<HttpResponse> HttpClient::GET() {
    auto transactionHandler = new TransactionHandler{this};
    auto txn = session_->newTransaction(transactionHandler);

    txn->sendHeaders(createHttpMessage(proxygen::HTTPMethod::GET));
    txn->sendEOM();

    requestPromise_.reset(new folly::Promise<HttpResponse>{});
    return requestPromise_->getFuture();
}


folly::Future<HttpResponse> HttpClient::POST(const std::string& content) {
    auto transactionHandler = new TransactionHandler{this};
    auto txn = session_->newTransaction(transactionHandler);

    txn->sendHeaders(createHttpMessage(proxygen::HTTPMethod::POST, content.size()));
    txn->sendBody(folly::IOBuf::copyBuffer(content));
    txn->sendEOM();

    requestPromise_.reset(new folly::Promise<HttpResponse>{});
    return requestPromise_->getFuture();
}


proxygen::HTTPMessage HttpClient::createHttpMessage(proxygen::HTTPMethod method,
                                                    size_t contentLength) {
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

    return httpMessage;
}


// CONNECT EVENT HANDLERS

void HttpClient::connectSuccess(HTTPUpstreamSession* session) {

    if (FLAGS_log_connect_time) {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> connectTime = end - connectStartTime_;
        LOG(INFO) << "HttpClient connect time: "
            << connectTime.count() << " milliseconds";
    }

    if (url_.isSecure()) {
        // TODO: sslHandshakeFollowup(session);
    }

    session_ = session;

    // Set receive buffer sizes
    session_->setFlowControl(65536, 65536, 65536);

    connectPromise_->setValue(folly::unit);
}


void HttpClient::connectError(const folly::AsyncSocketException& e) {
    LOG(ERROR) << "Failed to connect to " << url_.getHostAndPort();
    connectPromise_->setException(e);
}


// REQUEST EVENT HANDLERS

void HttpClient::requestComplete(HttpResponse httpResponse) noexcept {
    requestPromise_->setValue(std::move(httpResponse));
}


void HttpClient::requestError(const proxygen::HTTPException& error) noexcept {
    requestPromise_->setException(error);
}

