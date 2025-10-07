#include "common.h"
#include "ServerFactory.h"
#include "X402HandlerFactory.h"
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <curl/curl.h>



using namespace proxygen;

std::shared_ptr<HTTPServer> ServerFactory::createServerInstance(const ServerConfig& serverConfig) {



    HTTPServer::IPConfig ipConfig(
        folly::SocketAddress(serverConfig.bindIp(), serverConfig.httpPort().value(), true), HTTPServer::Protocol::HTTP);

    HTTPServerOptions options;
    spdlog::info("Creating server instance ...");
    options.threads = static_cast<size_t>(std::thread::hardware_concurrency());
    options.idleTimeout = std::chrono::milliseconds(60000);
    options.shutdownOn = {SIGINT, SIGTERM};
    options.handlerFactories = RequestHandlerChain()
            .addThen<X402HandlerFactory>()
            .build();

    auto server = std::make_shared<HTTPServer>(std::move(options));
    spdlog::info("Binding server to address...");
    server->bind({ipConfig});
    spdlog::info("Server instance created and bound successfully.");
    return server;
}
