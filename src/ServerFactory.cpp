#include "common.h"
#include "ServerFactory.h"
#include "X402HandlerFactory.h"
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <curl/curl.h>



using namespace proxygen;

std::shared_ptr<HTTPServer> ServerFactory::createServerInstance(std::string bindIP, uint64_t bindPort) {

    // Initialize libcurl once, for the whole process

    static bool curl_initialized = false;
    CURLcode rc = CURLE_OK;
    if (!curl_initialized) {
        rc = curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_initialized = true;
    }

    CHECK_STATE2(rc == CURLE_OK, "curl_global_init failed");

    HTTPServer::IPConfig ipConfig(
        folly::SocketAddress(bindIP, bindPort, true), HTTPServer::Protocol::HTTP);

    HTTPServerOptions options;
    LOG(INFO) << "Creating server instance ...";
    options.threads = static_cast<size_t>(std::thread::hardware_concurrency());
    options.idleTimeout = std::chrono::milliseconds(60000);
    options.shutdownOn = {SIGINT, SIGTERM};
    options.handlerFactories = RequestHandlerChain()
            .addThen<X402HandlerFactory>()
            .build();

    auto server = std::make_shared<HTTPServer>(std::move(options));
    LOG(INFO) << "Binding server to address...";
    server->bind({ipConfig});
    LOG(INFO) << "Server instance created and bound successfully.";
    return server;
}
