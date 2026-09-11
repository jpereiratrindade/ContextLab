#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace contextlab::domain {

struct Project {
    std::string id;
    std::string name;
    std::string kind;
    std::vector<std::string> research_domain;
    std::string stage;
    std::string object_of_study;
    std::string central_question;
    std::string engineering_question;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"name", name},
            {"kind", kind},
            {"research_domain", research_domain},
            {"stage", stage},
            {"object_of_study", object_of_study},
            {"central_question", central_question},
            {"engineering_question", engineering_question}
        };
    }
};

struct Concept {
    std::string id;
    std::string label;
    std::string role;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"label", label},
            {"role", role}
        };
    }
};

struct Relation {
    int64_t id{0};
    std::string subject;
    std::string predicate;
    std::string object;
    std::string document_id;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"subject", subject},
            {"predicate", predicate},
            {"object", object},
            {"document_id", document_id}
        };
    }
};

struct IngestionEvent {
    int64_t id{0};
    std::string timestamp;
    std::string source_file;
    std::string artifact_sha256;
    std::string document_id;
    std::string status;
    std::string message;
    nlohmann::json details = nlohmann::json::object();

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"timestamp", timestamp},
            {"source_file", source_file},
            {"artifact_sha256", artifact_sha256},
            {"document_id", document_id},
            {"status", status},
            {"message", message},
            {"details", details}
        };
    }
};

struct TextAnalysis {
    int64_t id{0};
    std::string document_id;
    std::string analyzed_at;
    uint64_t character_count{0};
    uint64_t word_count{0};
    uint64_t line_count{0};
    uint64_t section_count{0};
    std::vector<std::string> top_terms;
    std::vector<std::string> sections;
    std::string extraction_method;
    std::string source_digest;
    std::string sample_preview;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"document_id", document_id},
            {"analyzed_at", analyzed_at},
            {"character_count", character_count},
            {"word_count", word_count},
            {"line_count", line_count},
            {"section_count", section_count},
            {"top_terms", top_terms},
            {"sections", sections},
            {"extraction_method", extraction_method},
            {"source_digest", source_digest},
            {"sample_preview", sample_preview}
        };
    }
};

enum class RetrievalRoute {
    METADATA_SUFFICIENT,
    TEXT_INDEX_REQUIRED,
    DEEP_ANALYSIS_REQUIRED,
    UNRESOLVED
};

[[nodiscard]] constexpr std::string_view retrievalRouteToString(RetrievalRoute route) noexcept {
    switch (route) {
        case RetrievalRoute::METADATA_SUFFICIENT:    return "METADATA_SUFFICIENT";
        case RetrievalRoute::TEXT_INDEX_REQUIRED:   return "TEXT_INDEX_REQUIRED";
        case RetrievalRoute::DEEP_ANALYSIS_REQUIRED: return "DEEP_ANALYSIS_REQUIRED";
        case RetrievalRoute::UNRESOLVED:            return "UNRESOLVED";
    }
    return "UNKNOWN";
}

struct RetrievalDecision {
    int64_t id{0};
    std::string query;
    RetrievalRoute route{RetrievalRoute::METADATA_SUFFICIENT};
    std::string reason;
    std::vector<std::string> signals;
    std::vector<std::string> documents_considered;
    std::string chosen_path;
    std::string timestamp;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"query", query},
            {"route", retrievalRouteToString(route)},
            {"reason", reason},
            {"signals", signals},
            {"documents_considered", documents_considered},
            {"chosen_path", chosen_path},
            {"timestamp", timestamp}
        };
    }
};

struct SchemaDefinition {
    std::string id;
    std::string version;
    std::string digest;
    std::string source_path;
    nlohmann::json schema_json;
    bool active{true};
    std::string registered_at;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"version", version},
            {"digest", digest},
            {"source_path", source_path},
            {"schema_json", schema_json},
            {"active", active},
            {"registered_at", registered_at}
        };
    }
};

} // namespace contextlab::domain
