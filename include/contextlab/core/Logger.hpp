#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <mutex>
#include <chrono>
#include <format>
#include <nlohmann/json.hpp>

namespace contextlab::core {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

[[nodiscard]] constexpr std::string_view logLevelToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

struct LogEntry {
    LogLevel level{LogLevel::INFO};
    std::string event;
    std::string document_id;
    std::string artifact_sha256;
    std::string ingestion_id;
    std::string request_id;
    std::string message;
    nlohmann::json details = nlohmann::json::object();
};

class Logger {
public:
    static Logger& instance();

    void log(const LogEntry& entry);
    void info(std::string_view event, std::string_view message = "", const nlohmann::json& details = nlohmann::json::object());
    void warn(std::string_view event, std::string_view message = "", const nlohmann::json& details = nlohmann::json::object());
    void error(std::string_view event, std::string_view message = "", const nlohmann::json& details = nlohmann::json::object());
    void debug(std::string_view event, std::string_view message = "", const nlohmann::json& details = nlohmann::json::object());

    void setMinLevel(LogLevel level) noexcept { min_level_ = level; }

private:
    Logger() = default;
    LogLevel min_level_{LogLevel::INFO};
    std::mutex mutex_;
};

} // namespace contextlab::core
