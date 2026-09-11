#include "contextlab/ingest/JsonMetadataExtractor.hpp"

namespace contextlab::ingest {

bool JsonMetadataExtractor::canHandle(const std::filesystem::path& file_path, const std::string& media_type) const {
    if (media_type == "application/json") return true;
    std::string fname = file_path.filename().string();
    return fname.ends_with(".metadata.json") || fname.ends_with(".json");
}

core::Result<ExtractionResult> JsonMetadataExtractor::extract([[maybe_unused]] const std::filesystem::path& file_path, const std::string& content) const {
    ExtractionResult result;
    result.format_detected = "json_metadata";

    try {
        result.metadata_payload = nlohmann::json::parse(content);
        result.has_declared_context = true;
        result.raw_metadata_text = content;
        result.notes = "Direct JSON metadata parsed successfully";
        return core::makeOk(std::move(result));
    } catch (const std::exception& e) {
        return core::makeError(core::ErrorCode::METADATA_JSON_INVALID,
            std::string("Invalid JSON metadata file: ") + e.what());
    }
}

} // namespace contextlab::ingest
