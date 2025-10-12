#include "common.h"
#include "ServerFactory.h"
#include "X402HandlerFactory.h"
#include "crypto/CertManager.h"
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>



using namespace proxygen;

bool isRedHat() {
    return std::filesystem::exists("/etc/redhat-release");
}

bool isAlpine() {
    std::ifstream f("/etc/os-release");
    std::string line;
    while (std::getline(f, line)) {
        if (line.find("ID=alpine") != std::string::npos) {
            return true;
        }
    }
    return std::filesystem::exists("/etc/alpine-release");
}

void ServerFactory::addHttpServerToIPConfigs(const ServerConfig &serverConfig, std::vector<HTTPServer::IPConfig>& ipConfigs) {
    auto http = serverConfig.http();
    CHECK_STATE(http);
    ipConfigs.emplace_back(
        folly::SocketAddress(serverConfig.bindIp(),http->port(), true),
        HTTPServer::Protocol::HTTP
    );
}

void ServerFactory::addHTTPSServerToIpConfigs(const ServerConfig &serverConfig, std::vector<HTTPServer::IPConfig>& ipConfigs) {
    auto https = serverConfig.https();
    CHECK_STATE(https);
    auto sslCfg = CertManager::createAndValidateWangleSSLContext(https);

    HTTPServer::IPConfig config(
        folly::SocketAddress(serverConfig.bindIp(), https->port(), true),
        HTTPServer::Protocol::HTTP2); // we use HTTP2 for HTTPS, since this is default for industry now
    config.sslConfigs.push_back(sslCfg);
    ipConfigs.emplace_back(config);
}

std::shared_ptr<HTTPServer> ServerFactory::createServerInstance(const ServerConfig& serverConfig) {



    try {


        HTTPServerOptions options;
        spdlog::info("Creating server instance");

        auto factory = std::make_unique<X402HandlerFactory>(app_);

        options.threads = std::thread::hardware_concurrency();
        options.idleTimeout = std::chrono::milliseconds(60000);
        options.shutdownOn = {SIGINT, SIGTERM};
        options.handlerFactories = RequestHandlerChain()
                .addThen(std::move(factory))
                .build();


        std::vector<HTTPServer::IPConfig> ipConfigs;

        if (serverConfig.http() && serverConfig.http()->isEnabled()) {
            addHttpServerToIPConfigs(serverConfig, ipConfigs);
        }

        if (serverConfig.https() && serverConfig.https()->isEnabled()) {
            addHTTPSServerToIpConfigs(serverConfig, ipConfigs);
        }

        if (ipConfigs.empty()) {
            throw std::runtime_error("At least one of HTTP or HTTPS must"
                                     " be enabled in the server configuration.");
        }


        auto server = std::make_shared<HTTPServer>(std::move(options));

        for (const auto& config : ipConfigs) {
            spdlog::info("Binding to {}:{} [{}]",
                config.address.getAddressStr(),
                config.address.getPort(),
                config.sslConfigs.empty() ? " HTTP" : "HTTPS");
        }

        server->bind(ipConfigs);
        spdlog::info("Server instance created and bound successfully.");
        return server;
    } catch (const std::exception &ex) {
        RETHROW_NESTED;
    }
}
