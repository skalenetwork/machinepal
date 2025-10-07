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
    },
    "log": {
      "type": "object",
      "properties": {
        "level": {
          "type": "string",
          "enum": ["trace", "debug", "info", "warn", "error", "fatal"],
          "default": "info",
          "description": "Log verbosity level. Default: info. Override: LOG_LEVEL."
        },
        "json": {
          "type": "boolean",
          "default": false,
          "description": "Json format for logs. Default: false. Override: LOG_JSON."
        }
      },
      "description": "Optional logging configuration. Default log level is 'info' to stderr."
    }
  },
  "required": ["server", "facilitator"]
})";
