#pragma once

constexpr const char* MachinePayConfigSchemaJson = R"({
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "server": {
      "type": "object",
      "properties": {
        "bind_ip": { "type": "string" },
        "enable_http": { "type": "boolean" },
        "enable_https": { "type": "boolean" },
        "http_listen_port": { "type": "integer" },
        "https_listen_port": { "type": "integer" },
        "tls": {
          "type": "object",
          "properties": {
            "cert_file": { "type": "string" },
            "key_file": { "type": "string" },
            "key_pass_file": { "type": "string" },
            "ca_file": { "type": "string" }
          },
          "required": ["cert_file", "key_file", "key_pass_file"]
        }
      },
      "required": ["enable_http", "enable_https"]
    },
    "facilitator": {
      "type": "object",
      "properties": {
        "type": { "type": "string" },
        "base_url": { "type": "string" },
        "api_key_file": { "type": "string" }
      },
      "required": ["type", "base_url"]
    }
  },
  "required": ["server", "facilitator"]
})";

