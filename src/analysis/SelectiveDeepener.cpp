#include "contextlab/analysis/SelectiveDeepener.hpp"
#include "contextlab/core/Sha256.hpp"
#include "contextlab/core/Logger.hpp"
#include <sstream>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <format>

namespace contextlab::analysis {

SelectiveDeepener::SelectiveDeepener(ingest::ContentAddressableStore& cas, persistence::Repository& repo)
    : cas_(cas), repo_(repo) {}

std::vector<std::string> SelectiveDeepener::extractSections(std::string_view text) {
    std::vector<std::string> sections;
    std::istringstream stream{std::string(text)};
    std::string line;

    while (std::getline(stream, line)) {
        // Markdown headings
        if (line.starts_with("# ") || line.starts_with("## ") || line.starts_with("### ")) {
            auto first_char = line.find_first_not_of("# ");
            if (first_char != std::string::npos) {
                sections.push_back(line.substr(first_char));
            }
        }
        // LaTeX sections
        else if (line.find("\\section{") != std::string::npos) {
            auto start = line.find("\\section{") + 9;
            auto end = line.find('}', start);
            if (end != std::string::npos) {
                sections.push_back(line.substr(start, end - start));
            }
        }
    }
    return sections;
}

std::vector<std::string> SelectiveDeepener::extractTopTerms(std::string_view text, size_t limit) {
    static const std::unordered_set<std::string> stop_words = {
        "a", "o", "as", "os", "um", "uma", "uns", "umas", "de", "do", "da", "dos", "das",
        "em", "no", "na", "nos", "nas", "por", "para", "com", "sem", "sob", "sobre", "e",
        "ou", "mas", "que", "se", "como", "quando", "onde", "este", "esta", "estes", "estas",
        "esse", "essa", "esses", "essas", "aquele", "aquela", "aqueles", "aquelas", "isto",
        "isso", "aquilo", "seu", "sua", "seus", "suas", "meu", "minha", "nosso", "nossa",
        "the", "and", "or", "to", "of", "in", "on", "at", "by", "for", "with", "is", "are",
        "was", "were", "be", "this", "that", "it", "from", "an", "as"
    };

    std::unordered_map<std::string, size_t> counts;
    std::string current;

    for (char ch : text) {
        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '-') {
            current.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        } else if (!current.empty()) {
            if (current.size() >= 3 && stop_words.find(current) == stop_words.end()) {
                counts[current]++;
            }
            current.clear();
        }
    }
    if (!current.empty() && current.size() >= 3 && stop_words.find(current) == stop_words.end()) {
        counts[current]++;
    }

    std::vector<std::pair<std::string, size_t>> sorted_terms(counts.begin(), counts.end());
    std::sort(sorted_terms.begin(), sorted_terms.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    std::vector<std::string> top;
    for (size_t i = 0; i < std::min(sorted_terms.size(), limit); ++i) {
        top.push_back(sorted_terms[i].first);
    }
    return top;
}

core::Result<domain::TextAnalysis> SelectiveDeepener::deepen(const std::string& document_id) {
    auto doc_res = repo_.getDocument(document_id);
    if (!doc_res) return std::unexpected(doc_res.error());
    if (!doc_res->has_value()) {
        return core::makeError(core::ErrorCode::FILE_NOT_FOUND, "Document not found for deepening: " + document_id);
    }
    const auto& doc = **doc_res;

    auto artifacts_res = repo_.getArtifactsForDocument(document_id);
    if (!artifacts_res) return std::unexpected(artifacts_res.error());
    if (artifacts_res->empty()) {
        return core::makeError(core::ErrorCode::FILE_NOT_FOUND, "No artifacts linked to document: " + document_id);
    }

    const auto& artifact = (*artifacts_res)[0];
    if (artifact.media_type == "application/pdf") {
        // Contract section 18: If PDF text extraction is not available, explicitly declare TEXT_EXTRACTION_UNAVAILABLE_FOR_PDF
        return core::makeError(core::ErrorCode::TEXT_EXTRACTION_UNAVAILABLE, "TEXT_EXTRACTION_UNAVAILABLE_FOR_PDF");
    }

    auto content_res = cas_.readObjectString(artifact.sha256);
    if (!content_res) return std::unexpected(content_res.error());
    const std::string& raw_content = *content_res;

    // Clean metadata fence if markdown
    std::string text_body = raw_content;
    static const std::regex fence_regex(R"(```(?:context-metadata\+json|json\+context|context-metadata)\s*([\s\S]*?)```)");
    std::smatch match;
    if (std::regex_search(raw_content, match, fence_regex)) {
        text_body.erase(static_cast<size_t>(match.position()), static_cast<size_t>(match.length()));
    }

    uint64_t char_count = text_body.size();
    uint64_t word_count = 0;
    uint64_t line_count = 1;

    bool in_word = false;
    for (char c : text_body) {
        if (c == '\n') line_count++;
        if (std::isspace(static_cast<unsigned char>(c))) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            word_count++;
        }
    }

    auto sections = extractSections(text_body);
    auto top_terms = extractTopTerms(text_body, 12);

    std::string preview;
    if (text_body.size() > 300) {
        preview = text_body.substr(0, 300) + "...";
    } else {
        preview = text_body;
    }

    const auto now = std::chrono::system_clock::now();
    std::string now_str = std::format("{:%Y-%m-%d %H:%M:%S}", now);

    domain::TextAnalysis analysis{
        .document_id = document_id,
        .analyzed_at = now_str,
        .character_count = char_count,
        .word_count = word_count,
        .line_count = line_count,
        .section_count = sections.size(),
        .top_terms = std::move(top_terms),
        .sections = std::move(sections),
        .extraction_method = "native_utf8_structural_parser",
        .source_digest = artifact.sha256,
        .sample_preview = std::move(preview)
    };

    auto save_res = repo_.saveTextAnalysis(analysis);
    if (!save_res) return std::unexpected(save_res.error());

    // Index into SQLite FTS5 table
    auto fts_res = repo_.indexDocumentText(document_id, doc.title, text_body);
    if (!fts_res) return std::unexpected(fts_res.error());

    core::Logger::instance().info("DOCUMENT_DEEPENED", "Deepened text analysis completed for " + document_id, analysis.toJson());

    return core::makeOk(std::move(analysis));
}

} // namespace contextlab::analysis
