#include "LogConfig.h"
#include <stdexcept>

#include "ConfigLoader.h"

LogLevel parseLogLevel(const std::string& level) {
    if (level == "trace") return LogLevel::trace;
    if (level == "debug") return LogLevel::debug;
    if (level == "info") return LogLevel::info;
    if (level == "warn") return LogLevel::warn;
    if (level == "error") return LogLevel::error;
    if (level == "fatal") return LogLevel::fatal;
    throw std::invalid_argument("Invalid log level: " + level);
}

LogType parseLogType(const std::string& type) {
    if (type == "plain") return LogType::plain;
    if (type == "json") return LogType::json;
    throw std::invalid_argument("Invalid log type: " + type);
}

LogConfig::LogConfig(const std::string& level, const std::string& type)
    : level_(parseLogLevel(level)), type_(parseLogType(type)) {}
LogConfig::LogConfig(LogLevel level, LogType type)
    : level_(level), type_(type) {}
LogLevel LogConfig::level() const { return level_; }
LogType LogConfig::type() const { return type_; }
ptr<LogConfig> LogConfig::createDefault() {
    return ptr<LogConfig>(new LogConfig(LogLevel::info, LogType::plain));
}
ptr<LogConfig> LogConfig::createFromJson(const nlohmann::json& j, ptr<FileManager> fileManager)
{
    try {
        CHECK_STATE(fileManager);
        CHECK_STATE(j.is_object());
        return ptr<LogConfig>( new LogConfig( ConfigLoader::getStringWithDefault(j, "level", "info"),
            ConfigLoader::getStringWithDefault(j, "type", "plain")));
    } catch (const std::exception& ex) {
        RETHROW_NESTED;
    }
}
