#pragma once

constexpr const char* proxyConfigSchemaJson = R"({
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "frontend": {
      "type": "object",
      "properties": {
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
      "required": ["enable_http", "enable_https", "http_listen_port", "https_listen_port", "tls"]
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
  "required": ["frontend", "facilitator"]
})";

