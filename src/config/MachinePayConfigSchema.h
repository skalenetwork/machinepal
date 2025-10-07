#pragma once

constexpr const char* MachinePayConfigSchemaJson = R"({
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "server": {
      "type": "object",
      "properties": {
        "bind_ip": { "type": "string" },
        "http": {
          "type": "object",
          "properties": {
            "enabled": { "type": "boolean" },
            "port": { "type": "integer" }
          },
          "required": ["port"]
        },
        "https": {
          "type": "object",
          "properties": {
            "enabled": { "type": "boolean" },
            "port": { "type": "integer" },
            "cert_file": { "type": "string" },
            "key_file": { "type": "string" },
            "key_pass_file": { "type": "string" },
            "ca_file": { "type": "string" }
          },
          "required": ["port", "cert_file", "key_file", "key_pass_file"]
        }
      },
      "required": ["http", "https"]
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
          "description": "Log verbosity level. Default: info. Override: LOG_LEVEL.",
          "default": "info"
        },
        "type": {
          "type": "string",
          "enum": ["plain", "json"],
          "description": "Type for logs. Default: plain. Override: LOG_TYPE.",
          "default": "plain"
        }
      },
      "description": "Optional logging configuration. Default log level is 'info' to stderr."
    }
  },
  "required": ["server", "facilitator"]
})";
