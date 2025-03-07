#include "HttpHandlerFactory.h"

#include <folly/init/Init.h>
#include <folly/String.h>
#include <proxygen/httpserver/HTTPServer.h>


static void run();


int main(int argc, char *argv[]) {
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;

    folly::Init init(&argc, &argv);

    try {
        run();
    } catch (const std::exception& e) {
        LOG(ERROR) << e.what();
        return 1;
    }

    return 0;
}


void run() {
    proxygen::HTTPServerOptions httpServerOptions;
    httpServerOptions.shutdownOn = {SIGINT, SIGTERM};
    httpServerOptions.handlerFactories = proxygen::RequestHandlerChain()
        .addThen<HttpHandlerFactory>()
        .build();

    proxygen::HTTPServer server{std::move(httpServerOptions)};

    constexpr auto allowNameLookup = true;
    constexpr auto HTTP = proxygen::HTTPServer::Protocol::HTTP;
    constexpr uint16_t httpPort = 8080;
    server.bind({
        {folly::SocketAddress{"0.0.0.0", httpPort, allowNameLookup}, HTTP},
    });

    LOG(INFO) << "Starting HTTP hello server on port " << httpPort;
    server.start();
    LOG(INFO) << "HTTP server exited";
}
