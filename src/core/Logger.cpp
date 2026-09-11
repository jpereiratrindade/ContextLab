#include "contextlab/core/Logger.hpp"

namespace contextlab::core {

Logger& Logger::instance() {
    static Logger s_logger;
    return s_logger;
}

void Logger::log(const LogEntry& entry) {
    if (static_cast<int>(entry.level) < static_cast<int>(min_level_)) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto time_str = std::format("{:%Y-%m-%d %H:%M:%S}", now);

    nlohmann::json record = {
        {"timestamp", time_str},
        {"level", logLevelToString(entry.level)},
        {"event", entry.event}
    };

    if (!entry.document_id.empty()) record["document_id"] = entry.document_id;
    if (!entry.artifact_sha256.empty()) record["artifact_sha256"] = entry.artifact_sha256;
    if (!entry.ingestion_id.empty()) record["ingestion_id"] = entry.ingestion_id;
    if (!entry.request_id.empty()) record["request_id"] = entry.request_id;
    if (!entry.message.empty()) record["message"] = entry.message;
    if (!entry.details.empty()) record["details"] = entry.details;

    std::lock_guard<std::mutex> lock(mutex_);
    std::cerr << record.dump() << "\n";
}

void Logger::info(std::string_view event, std::string_view message, const nlohmann::json& details) {
    log(LogEntry{
        .level = LogLevel::INFO,
        .event = std::string(event),
        .message = std::string(message),
        .details = details
    });
}

void Logger::warn(std::string_view event, std::string_view message, const nlohmann::json& details) {
    log(LogEntry{
        .level = LogLevel::WARN,
        .event = std::string(event),
        .message = std::string(message),
        .details = details
    });
}

void Logger::error(std::string_view event, std::string_view message, const nlohmann::json& details) {
    log(LogEntry{
        .level = LogLevel::ERROR,
        .event = std::string(event),
        .message = std::string(message),
        .details = details
    });
}

void Logger::debug(std::string_view event, std::string_view message, const nlohmann::json& details) {
    log(LogEntry{
        .level = LogLevel::DEBUG,
        .event = std::string(event),
        .message = std::string(message),
        .details = details
    });
}

} // namespace contextlab::core
