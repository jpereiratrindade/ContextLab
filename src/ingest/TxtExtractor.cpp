#include "contextlab/ingest/TxtExtractor.hpp"

namespace contextlab::ingest {

bool TxtExtractor::canHandle(const std::filesystem::path& file_path, const std::string& media_type) const {
    if (media_type == "text/plain") return true;
    std::string ext = file_path.extension().string();
    return ext == ".txt";
}

core::Result<ExtractionResult> TxtExtractor::extract([[maybe_unused]] const std::filesystem::path& file_path, const std::string& content) const {
    ExtractionResult result;
    result.format_detected = "text";
    result.has_declared_context = false;
    result.raw_body_text = content;
    result.notes = "Plain text document; declared metadata is absent";
    return core::makeOk(std::move(result));
}

} // namespace contextlab::ingest
