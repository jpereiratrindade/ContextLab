#include "contextlab/retrieval/SearchEngine.hpp"
#include "contextlab/retrieval/RetrievalRouter.hpp"
#include <algorithm>
#include <sstream>

namespace contextlab::retrieval {

namespace {

std::string toLower(std::string_view str) {
    std::string out(str);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return out;
}

std::vector<std::string> tokenize(std::string_view str) {
    std::vector<std::string> tokens;
    std::string current;
    for (char c : str) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
            current.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else if (!current.empty()) {
            if (current.size() >= 2) tokens.push_back(current);
            current.clear();
        }
    }
    if (current.size() >= 2) tokens.push_back(current);
    return tokens;
}

} // namespace

SearchEngine::SearchEngine(persistence::Repository& repo) : repo_(repo) {}

core::Result<SearchResponse> SearchEngine::search(const std::string& query, const std::string& mode) {
    auto all_docs_res = repo_.getAllDocuments();
    if (!all_docs_res) return std::unexpected(all_docs_res.error());
    const auto& docs = *all_docs_res;

    std::vector<std::string> doc_ids;
    for (const auto& d : docs) doc_ids.push_back(d.id);

    domain::RetrievalDecision decision = RetrievalRouter::classifyQuery(query, doc_ids);
    if (mode == "metadata") {
        decision.route = domain::RetrievalRoute::METADATA_SUFFICIENT;
        decision.reason = "Explicit operator override to metadata-only search.";
    } else if (mode == "text") {
        decision.route = domain::RetrievalRoute::TEXT_INDEX_REQUIRED;
        decision.reason = "Explicit operator override to full-text FTS5 search.";
    }

    // Record decision in DB
    (void)repo_.recordRetrievalDecision(decision);

    SearchResponse response{
        .query = query,
        .route = std::string(domain::retrievalRouteToString(decision.route)),
        .reason = decision.reason,
        .signals = decision.signals,
        .text_analysis_performed = false,
        .results = {}
    };

    auto tokens = tokenize(query);
    std::string q_lower = toLower(query);

    // 1. Metadata-first search pass
    for (const auto& doc : docs) {
        std::string doc_id_lower = toLower(doc.id);
        std::string title_lower = toLower(doc.title);
        std::string proj_lower = toLower(doc.primary_project);
        std::string epistemic_lower = toLower(doc.epistemic_status);

        bool matched = false;
        std::string explanation;

        if (doc_id_lower.find(q_lower) != std::string::npos || q_lower.find(doc_id_lower) != std::string::npos) {
            matched = true;
            explanation = "Matched exact Document ID: " + doc.id;
        } else if (title_lower.find(q_lower) != std::string::npos) {
            matched = true;
            explanation = "Matched in document title: \"" + doc.title + "\"";
        } else if (proj_lower.find(q_lower) != std::string::npos) {
            matched = true;
            explanation = "Matched primary project: " + doc.primary_project;
        } else if (epistemic_lower.find(q_lower) != std::string::npos) {
            matched = true;
            explanation = "Matched epistemic status: " + doc.epistemic_status;
        } else {
            // Check tokens
            for (const auto& tok : tokens) {
                if (doc_id_lower.find(tok) != std::string::npos) {
                    matched = true;
                    explanation = "Matched term '" + tok + "' in Document ID";
                    break;
                }
                if (title_lower.find(tok) != std::string::npos) {
                    matched = true;
                    explanation = "Matched term '" + tok + "' in title";
                    break;
                }
                if (proj_lower.find(tok) != std::string::npos) {
                    matched = true;
                    explanation = "Matched term '" + tok + "' in project";
                    break;
                }
            }
        }

        // Check envelope payloads & provenance
        if (!matched) {
            auto envs_res = repo_.getMetadataEnvelopes(doc.id);
            if (envs_res && !envs_res->empty()) {
                for (const auto& env : *envs_res) {
                    std::string payload_str = env.payload.dump();
                    std::string payload_lower = toLower(payload_str);
                    for (const auto& tok : tokens) {
                        if (payload_lower.find(tok) != std::string::npos) {
                            matched = true;
                            explanation = "Matched term '" + tok + "' in declared metadata payload (" + env.schema_id + ")";
                            break;
                        }
                    }
                    if (matched) break;
                }
            }
        }

        // Check relations
        if (!matched) {
            auto rels_res = repo_.getRelationsForDocument(doc.id);
            if (rels_res && !rels_res->empty()) {
                for (const auto& rel : *rels_res) {
                    std::string rel_str = rel.subject + " " + rel.predicate + " " + rel.object;
                    std::string rel_lower = toLower(rel_str);
                    for (const auto& tok : tokens) {
                        if (rel_lower.find(tok) != std::string::npos) {
                            matched = true;
                            explanation = "Matched relation: " + rel.predicate + " -> " + rel.object;
                            break;
                        }
                    }
                    if (matched) break;
                }
            }
        }

        if (matched) {
            response.results.push_back(SearchResultItem{
                .document_id = doc.id,
                .title = doc.title,
                .primary_project = doc.primary_project,
                .epistemic_status = doc.epistemic_status,
                .matched_by = "metadata",
                .explanation = explanation,
                .snippet = "",
                .score = 1.0
            });
        }
    }

    // 2. FTS5 Search pass if text mode or deep analysis
    if (decision.route == domain::RetrievalRoute::TEXT_INDEX_REQUIRED ||
        decision.route == domain::RetrievalRoute::DEEP_ANALYSIS_REQUIRED ||
        mode == "text") {

        response.text_analysis_performed = true;
        auto fts_res = repo_.searchFts(query);
        if (fts_res && !fts_res->empty()) {
            for (const auto& [doc_id, snippet] : *fts_res) {
                // Check if already in results
                bool found = false;
                for (auto& item : response.results) {
                    if (item.document_id == doc_id) {
                        item.matched_by = "metadata + text_index";
                        item.snippet = snippet;
                        item.explanation += " (Also matched in indexed text body)";
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    auto d_res = repo_.getDocument(doc_id);
                    if (d_res && d_res->has_value()) {
                        response.results.push_back(SearchResultItem{
                            .document_id = doc_id,
                            .title = (*d_res)->title,
                            .primary_project = (*d_res)->primary_project,
                            .epistemic_status = (*d_res)->epistemic_status,
                            .matched_by = "fts5_text",
                            .explanation = "Matched through SQLite FTS5 full-text index",
                            .snippet = snippet,
                            .score = 0.85
                        });
                    }
                }
            }
        }
    }

    return core::makeOk(std::move(response));
}

} // namespace contextlab::retrieval
