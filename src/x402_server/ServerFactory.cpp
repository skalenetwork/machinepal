#include "common.h"
#include "ServerFactory.h"
#include "X402HandlerFactory.h"
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



static void checkWellFormedPEM(const std::string& certPath, const std::string& keyPath) {

    FILE* certFile = fopen(certPath.c_str(), "r");
    if (!certFile)
        throw std::runtime_error("Cannot open certificate file: " + certPath);

    X509* cert = PEM_read_X509(certFile, nullptr, nullptr, nullptr);
    fclose(certFile);
    if (!cert)
        throw std::runtime_error("Certificate file is not a well-formed PEM: " + certPath);

    FILE* keyFile = fopen(keyPath.c_str(), "r");
    if (!keyFile)
        throw std::runtime_error("Cannot open key file: " + keyPath);

    EVP_PKEY* pkey = PEM_read_PrivateKey(keyFile, nullptr, nullptr, nullptr);
    fclose(keyFile);
    if (!pkey)
        throw std::runtime_error("Key file is not a well-formed PEM: " + keyPath);

    X509_free(cert);
    EVP_PKEY_free(pkey);
}

std::shared_ptr<HTTPServer> ServerFactory::createServerInstance(const ServerConfig& serverConfig) {



    try {
        std::vector<HTTPServer::IPConfig> ipConfigs;
        if (serverConfig.http() && serverConfig.http()->isEnabled()) {
            ipConfigs.emplace_back(
                folly::SocketAddress(serverConfig.bindIp(), serverConfig.http()->port(), true),
                HTTPServer::Protocol::HTTP
            );
        }



        if (auto https = serverConfig.https(); https && https->isEnabled()) {
            wangle::SSLContextConfig sslCfg;

            CHECK_STATE(!https->keyFile().empty());
            CHECK_STATE(!https->certFile().empty());

            auto keyPassPath = https->keyPassFile() ? https->keyPassFile().value() : "";

            checkWellFormedPEM(https->certFile(), https->keyFile());

            sslCfg.addCertificate(https->certFile(), https->keyFile(),
                keyPassPath);

            if (https->caFile() && !https->caFile()->empty()) {
                sslCfg.clientCAFile = https->caFile().value();
            } else {
                if (isRedHat()) {
                    sslCfg.clientCAFile = "/etc/pki/tls/certs/ca-bundle.crt";
                } else if (isAlpine()) {
                    sslCfg.clientCAFile = "/etc/ssl/cert.pem";
                } else {
                    sslCfg.clientCAFile  = "/etc/ssl/certs/ca-certificates.crt";
                }
            }


            if (!std::filesystem::exists(sslCfg.clientCAFile)) {
                throw std::runtime_error("CA file does not exist: " +
                    sslCfg.clientCAFile);
            }


            HTTPServer::IPConfig config(
                folly::SocketAddress(serverConfig.bindIp(), https->port(), true),
                HTTPServer::Protocol::HTTP);

            config.sslConfigs.push_back(sslCfg);

            ipConfigs.emplace_back(config);
        }

        if (ipConfigs.empty()) {
            throw std::runtime_error("At least one of HTTP or HTTPS must"
                                     " be enabled in the server configuration.");
        }

        HTTPServerOptions options;
        spdlog::info("Creating server instance");
        options.threads = static_cast<size_t>(std::thread::hardware_concurrency());
        options.idleTimeout = std::chrono::milliseconds(60000);
        options.shutdownOn = {SIGINT, SIGTERM};
        options.handlerFactories = RequestHandlerChain()
                .addThen<X402HandlerFactory>()
                .build();

        auto server = std::make_shared<HTTPServer>(std::move(options));

        for (const auto& config : ipConfigs) {
            spdlog::info("Binding to {}:{} [{}]",
                config.address.getAddressStr(),
                config.address.getPort(),
                config.protocol == HTTPServer::Protocol::HTTP ? "HTTP" : "HTTPS");
        }

        server->bind(ipConfigs);
        spdlog::info("Server instance created and bound successfully.");
        return server;
    } catch (const std::exception &ex) {
        RETHROW_NESTED("ServerFactory::createServerInstance failed: ");
    }
}
