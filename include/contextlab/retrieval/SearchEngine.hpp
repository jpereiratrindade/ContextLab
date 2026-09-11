#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "contextlab/core/Result.hpp"
#include "contextlab/domain/DomainModels.hpp"
#include "contextlab/persistence/Repository.hpp"

namespace contextlab::retrieval {

struct SearchResultItem {
    std::string document_id;
    std::string title;
    std::string primary_project;
    std::string epistemic_status;
    std::string matched_by; // "metadata", "fts5_text", "relation"
    std::string explanation;
    std::string snippet;
    double score{1.0};

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"document_id", document_id},
            {"title", title},
            {"primary_project", primary_project},
            {"epistemic_status", epistemic_status},
            {"matched_by", matched_by},
            {"explanation", explanation},
            {"snippet", snippet},
            {"score", score}
        };
    }
};

struct SearchResponse {
    std::string query;
    std::string route;
    std::string reason;
    std::vector<std::string> signals;
    bool text_analysis_performed{false};
    std::vector<SearchResultItem> results;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json res_arr = nlohmann::json::array();
        for (const auto& item : results) {
            res_arr.push_back(item.toJson());
        }
        return {
            {"query", query},
            {"route", route},
            {"reason", reason},
            {"signals", signals},
            {"text_analysis_performed", text_analysis_performed},
            {"results", res_arr}
        };
    }
};

class SearchEngine {
public:
    explicit SearchEngine(persistence::Repository& repo);

    [[nodiscard]] core::Result<SearchResponse> search(const std::string& query, const std::string& mode = "auto");

private:
    persistence::Repository& repo_;
};

} // namespace contextlab::retrieval
