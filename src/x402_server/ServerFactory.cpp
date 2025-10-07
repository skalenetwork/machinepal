#include "common.h"
#include "ServerFactory.h"
#include "X402HandlerFactory.h"
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <curl/curl.h>



using namespace proxygen;

std::shared_ptr<HTTPServer> ServerFactory::createServerInstance(const ServerConfig& serverConfig) {



    try {
        std::vector<HTTPServer::IPConfig> ipConfigs;
        if (serverConfig.http() && serverConfig.http()->isEnabled()) {
            ipConfigs.emplace_back(
                folly::SocketAddress(serverConfig.bindIp(), serverConfig.http()->port(), true),
                HTTPServer::Protocol::HTTP
            );
        }



        HTTPServerOptions options;
        spdlog::info("Creating server instance ...");
        options.threads = static_cast<size_t>(std::thread::hardware_concurrency());
        options.idleTimeout = std::chrono::milliseconds(60000);
        options.shutdownOn = {SIGINT, SIGTERM};
        options.handlerFactories = RequestHandlerChain()
                .addThen<X402HandlerFactory>()
                .build();


        if (auto https = serverConfig.https(); https && https->isEnabled()) {
            wangle::SSLContextConfig sslCfg;
            sslCfg.addCertificate(https->certFile(), https->keyFile(),
                https->keyPassFile());
            if (https->caFile() && !https->caFile()->empty()) {
                sslCfg.clientCAFile = *https->caFile();
            }
            // If you have a chain file, set sslCfg.chainFile = ...;

            HTTPServer::IPConfig config(
                folly::SocketAddress(serverConfig.bindIp(), https->port(), true),
                HTTPServer::Protocol::HTTP);

            config.sslConfigs.push_back(sslCfg);

            ipConfigs.emplace_back(config);


        }


        auto server = std::make_shared<HTTPServer>(std::move(options));
        spdlog::info("Binding server to address(es)...");
        server->bind(ipConfigs);
        spdlog::info("Server instance created and bound successfully.");
        return server;
    } catch (const std::exception &ex) {
        RETHROW_NESTED("ServerFactory::createServerInstance failed: ");
    }
    // make compiler happy
    return nullptr;
}
