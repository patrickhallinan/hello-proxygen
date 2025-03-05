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


namespace {
}


HttpClient::HttpClient(EventBase* eb,
                       std::chrono::milliseconds defaultTimeout,
                       const HTTPHeaders& headers)
    : eb_{eb}
    , headers_{headers} {

    // uses HHWheelTimer from EventBase
    proxygen::WheelTimerInstance timer{defaultTimeout, eb};

    httpConnector_ = std::make_unique<proxygen::HTTPConnector>(this, timer);
}


folly::Future<folly::Unit> HttpClient::connect(const std::string& host,
                                               const uint16_t port) {

    connectStartTime_ = std::chrono::high_resolution_clock::now();

    folly::SocketAddress socketAddress{host, port, /*allowNameLookup*/true};

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

    host_ = host;
    port_ = port;

    return connectPromise_->getFuture();
}


folly::Future<HttpResponse> HttpClient::GET(const std::string& path) {

    auto transactionHandler = new TransactionHandler{};
    auto txn = session_->newTransaction(transactionHandler);

    auto httpMessage = createHttpMessage(proxygen::HTTPMethod::GET,
                                         path);

    txn->sendHeaders(std::move(httpMessage));
    txn->sendEOM();

    return transactionHandler->getFuture();
}


folly::Future<HttpResponse> HttpClient::POST(const std::string& path,
                                             const std::string& content) {

    auto transactionHandler = new TransactionHandler{};
    auto txn = session_->newTransaction(transactionHandler);

    auto httpMessage = createHttpMessage(proxygen::HTTPMethod::POST,
                                         path,
                                         content.size());

    txn->sendHeaders(std::move(httpMessage));
    txn->sendBody(folly::IOBuf::copyBuffer(content));
    txn->sendEOM();

    return transactionHandler->getFuture();
}


proxygen::HTTPMessage HttpClient::createHttpMessage(proxygen::HTTPMethod method,
                                                    const std::string& path,
                                                    size_t contentLength) {
    proxygen::HTTPMessage httpMessage;



    // example:  GET /path HTTP/1.1
    httpMessage.setMethod(method);
    httpMessage.setURL(path);
    httpMessage.setHTTPVersion(1, 1);
    httpMessage.setSecure(false);

    headers_.forEach([&httpMessage](const string& header, const string& val) {
        httpMessage.getHeaders().add(header, val);
    });

    if (! httpMessage.getHeaders().getNumberOfValues(HTTP_HEADER_USER_AGENT)) {
        httpMessage.getHeaders().add(HTTP_HEADER_USER_AGENT, "http-client");
    }

    if (! httpMessage.getHeaders().getNumberOfValues(HTTP_HEADER_HOST)) {
		const auto endpoint = fmt::format("{}:{}", host_, port_);
        httpMessage.getHeaders().add(HTTP_HEADER_HOST, endpoint);
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

    //if (isSecure_) {
        // TODO: sslHandshakeFollowup(session);
    //}

    session_ = session;

    // Set receive buffer sizes
    session_->setFlowControl(65536, 65536, 65536);

    connectPromise_->setValue(folly::unit);
}


void HttpClient::connectError(const folly::AsyncSocketException& e) {
    LOG(ERROR) << "Failed to connect to " << host_ << ":" << port_;
    connectPromise_->setException(e);
}

