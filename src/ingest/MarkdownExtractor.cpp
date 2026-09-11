#include "contextlab/ingest/MarkdownExtractor.hpp"
#include <regex>

namespace contextlab::ingest {

bool MarkdownExtractor::canHandle(const std::filesystem::path& file_path, const std::string& media_type) const {
    if (media_type == "text/markdown") return true;
    std::string ext = file_path.extension().string();
    return ext == ".md" || ext == ".markdown";
}

core::Result<ExtractionResult> MarkdownExtractor::extract([[maybe_unused]] const std::filesystem::path& file_path, const std::string& content) const {
    ExtractionResult result;
    result.format_detected = "markdown";

    // Regex to match ```context-metadata+json ... ``` or ```json+context ... ```
    static const std::regex fence_regex(R"(```(?:context-metadata\+json|json\+context|context-metadata)\s*([\s\S]*?)```)");

    std::smatch match;
    if (std::regex_search(content, match, fence_regex)) {
        std::string json_str = match[1].str();
        result.raw_metadata_text = json_str;

        try {
            result.metadata_payload = nlohmann::json::parse(json_str);
            result.has_declared_context = true;
            result.notes = "Extracted fenced context-metadata+json block successfully";

            // Remove metadata fence from body text for clean deferred reading
            std::string body = content;
            body.erase(static_cast<size_t>(match.position()), static_cast<size_t>(match.length()));
            result.raw_body_text = std::move(body);

            return core::makeOk(std::move(result));
        } catch (const std::exception& e) {
            return core::makeError(core::ErrorCode::METADATA_JSON_INVALID,
                std::string("Found context metadata fence but JSON is invalid: ") + e.what(),
                {{"raw_snippet", json_str.substr(0, std::min<size_t>(json_str.size(), 200))}});
        }
    }

    result.has_declared_context = false;
    result.raw_body_text = content;
    result.notes = "No context-metadata+json fence found in markdown";
    return core::makeOk(std::move(result));
}

} // namespace contextlab::ingest
