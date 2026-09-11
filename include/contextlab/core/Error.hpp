#pragma once

#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace contextlab::core {

enum class ErrorCode {
    FILE_NOT_FOUND,
    FORMAT_UNSUPPORTED,
    METADATA_NOT_FOUND,
    METADATA_AMBIGUOUS,
    METADATA_JSON_INVALID,
    METADATA_SCHEMA_UNKNOWN,
    METADATA_SCHEMA_INVALID,
    DOCUMENT_ID_CONFLICT,
    VERSION_CONFLICT,
    PDF_ATTACHMENT_ERROR,
    TEXT_EXTRACTION_UNAVAILABLE,
    DATABASE_ERROR,
    INVALID_COMMAND,
    INVALID_ARGUMENT,
    PERMISSION_DENIED,
    INTERNAL_ERROR
};

[[nodiscard]] constexpr std::string_view errorCodeToString(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
        case ErrorCode::FORMAT_UNSUPPORTED: return "FORMAT_UNSUPPORTED";
        case ErrorCode::METADATA_NOT_FOUND: return "METADATA_NOT_FOUND";
        case ErrorCode::METADATA_AMBIGUOUS: return "METADATA_AMBIGUOUS";
        case ErrorCode::METADATA_JSON_INVALID: return "METADATA_JSON_INVALID";
        case ErrorCode::METADATA_SCHEMA_UNKNOWN: return "METADATA_SCHEMA_UNKNOWN";
        case ErrorCode::METADATA_SCHEMA_INVALID: return "METADATA_SCHEMA_INVALID";
        case ErrorCode::DOCUMENT_ID_CONFLICT: return "DOCUMENT_ID_CONFLICT";
        case ErrorCode::VERSION_CONFLICT: return "VERSION_CONFLICT";
        case ErrorCode::PDF_ATTACHMENT_ERROR: return "PDF_ATTACHMENT_ERROR";
        case ErrorCode::TEXT_EXTRACTION_UNAVAILABLE: return "TEXT_EXTRACTION_UNAVAILABLE";
        case ErrorCode::DATABASE_ERROR: return "DATABASE_ERROR";
        case ErrorCode::INVALID_COMMAND: return "INVALID_COMMAND";
        case ErrorCode::INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case ErrorCode::PERMISSION_DENIED: return "PERMISSION_DENIED";
        case ErrorCode::INTERNAL_ERROR: return "INTERNAL_ERROR";
    }
    return "UNKNOWN_ERROR";
}

struct Error {
    ErrorCode code{ErrorCode::INTERNAL_ERROR};
    std::string message;
    nlohmann::json details = nlohmann::json::object();
    bool recoverable{true};

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"error", {
                {"code", errorCodeToString(code)},
                {"message", message},
                {"details", details},
                {"recoverable", recoverable}
            }}
        };
    }
};

} // namespace contextlab::core
