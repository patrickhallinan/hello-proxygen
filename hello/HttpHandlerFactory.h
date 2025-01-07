#pragma once

#include "HttpHandler.h"

#include <proxygen/httpserver/HTTPServer.h>

#include <glog/logging.h>


class HttpHandlerFactory : public proxygen::RequestHandlerFactory {
    thread_local static folly::EventBase* eb_;
public:
    // Called for each handler event base thread
    void onServerStart(folly::EventBase* eb) noexcept override {
        eb_ = eb;
        LOG(INFO) << "onServerStart()";
    }

    void onServerStop() noexcept override {
        eb_ = nullptr;
        LOG(INFO) << "onServerStop()";
    }

    proxygen::RequestHandler* onRequest(proxygen::RequestHandler* handler,
                                        proxygen::HTTPMessage* httpMessage) noexcept override {

        return new HttpHandler{eb_};
    }
};
