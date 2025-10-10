#pragma once
#include <string>
#include <memory>
#include "common.h"
#include "nlohmann/json_fwd.hpp"

enum class LogLevel {
    trace, debug, info, warn, error, fatal
};

enum class LogType {
    plain, json
};

LogLevel parseLogLevel(const std::string& level);
LogType parseLogType(const std::string& type);

class FileManager;

class LogConfig {
    LogLevel level_;
    LogType type_;
public:
    LogConfig(const std::string& level, const std::string& type);
    LogConfig(LogLevel level, LogType type);
    LogLevel level() const;
    LogType type() const;
    static ptr<LogConfig> createDefault();
    static ptr<LogConfig> createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager);
};

