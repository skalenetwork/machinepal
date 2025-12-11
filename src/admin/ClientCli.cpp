#include "MachinePayCommon.h"
#include "ClientCli.h"
#include <CLI/CLI.hpp>
#include <boost/url.hpp> // Requires Boost 1.81+

#include "payment/datastructures/PaymentPayload.h"
#include "x402_client/X402Client.h"


void ClientCli::addClientSubcommand(CLI::App& app, ClientConfig& config) {
    auto* client = app.add_subcommand("client", "Client commands");

    auto boostUrlValidator = [](const std::string& val) -> std::string {
        // 1. Attempt to parse the URI
        boost::system::result<boost::urls::url_view> result = boost::urls::parse_uri(val);

        if (result.has_error()) {
            return "Invalid URL format: " + result.error().message();
        }

        if (result->scheme_id() != boost::urls::scheme::http &&
            result->scheme_id() != boost::urls::scheme::https) {
            return "URL must use http or https scheme";
            }

        if (!result->has_authority()) {
            return "URL must contain a valid host";
        }

        return ""; // Empty string means validation passed
    };

    client->add_option("-u,--url", config.url, "URL for the endpoint")
          ->type_name("URL")
          ->check(boostUrlValidator) // Apply the Boost validator here
          ->required();

    // 3. Capture option pointers...
    client->add_option("-m,--method", config.method, "HTTP method")
          ->default_val("GET")
          ->check(CLI::IsMember({"GET", "POST"}, CLI::ignore_case));

    client->add_option("-p,--payload-file", config.payload, "JSON payload file")
          ->type_name("JSON_STRING");
}

int ClientCli::runClientCommand(const ClientConfig &config) {
    ptr<PaymentPayload> payload = nullptr;

    try {
        X402Client client;

        if (config.payload != "") {
            payload = PaymentPayload::fromJson(json::parse(config.payload));
        }

        client.doX402Request(config.method, config.url, payload, std::nullopt);
    } catch (const std::exception &ex) {
        spdlog::error("Exception running client command: {}", ex.what());
        return 1;
    }

    return 0;

}
