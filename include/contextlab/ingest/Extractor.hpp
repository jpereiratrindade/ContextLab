#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "contextlab/core/Result.hpp"

namespace contextlab::ingest {

struct ExtractionResult {
    bool has_declared_context{false};
    nlohmann::json metadata_payload = nlohmann::json::object();
    std::string raw_metadata_text;
    std::string raw_body_text;
    std::string format_detected;
    std::string notes;
    std::vector<std::string> ambiguous_candidates;
    bool is_ambiguous{false};
};

class IExtractor {
public:
    virtual ~IExtractor() = default;
    [[nodiscard]] virtual bool canHandle(const std::filesystem::path& file_path, const std::string& media_type) const = 0;
    [[nodiscard]] virtual core::Result<ExtractionResult> extract(const std::filesystem::path& file_path, const std::string& content) const = 0;
};

} // namespace contextlab::ingest
