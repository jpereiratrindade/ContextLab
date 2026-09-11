#include "contextlab/ingest/LatexExtractor.hpp"
#include <regex>

namespace contextlab::ingest {

bool LatexExtractor::canHandle(const std::filesystem::path& file_path, const std::string& media_type) const {
    if (media_type == "application/x-tex") return true;
    std::string ext = file_path.extension().string();
    return ext == ".tex" || ext == ".latex";
}

core::Result<ExtractionResult> LatexExtractor::extract([[maybe_unused]] const std::filesystem::path& file_path, const std::string& content) const {
    ExtractionResult result;
    result.format_detected = "latex";

    // Match \begin{filecontents*}{*.metadata.json} ... \end{filecontents*}
    static const std::regex filecontents_regex(
        R"(\\begin\{filecontents\*?\}\{([^}]+?\.metadata\.json)\}\s*([\s\S]*?)\\end\{filecontents\*?\})"
    );

    std::smatch match;
    if (std::regex_search(content, match, filecontents_regex)) {
        std::string filename = match[1].str();
        std::string json_str = match[2].str();
        result.raw_metadata_text = json_str;

        try {
            result.metadata_payload = nlohmann::json::parse(json_str);
            result.has_declared_context = true;
            result.notes = "Extracted embedded filecontents metadata block: " + filename;

            std::string body = content;
            body.erase(static_cast<size_t>(match.position()), static_cast<size_t>(match.length()));
            result.raw_body_text = std::move(body);

            return core::makeOk(std::move(result));
        } catch (const std::exception& e) {
            return core::makeError(core::ErrorCode::METADATA_JSON_INVALID,
                std::string("LaTeX filecontents JSON invalid: ") + e.what());
        }
    }

    result.has_declared_context = false;
    result.raw_body_text = content;
    result.notes = "No .metadata.json filecontents block found in LaTeX";
    return core::makeOk(std::move(result));
}

} // namespace contextlab::ingest
