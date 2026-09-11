#include "contextlab/retrieval/RetrievalRouter.hpp"
#include <algorithm>
#include <chrono>
#include <format>

namespace contextlab::retrieval {

namespace {

std::string toLower(std::string_view str) {
    std::string out(str);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return out;
}

bool containsAny(const std::string& text, const std::vector<std::string>& keywords) {
    for (const auto& kw : keywords) {
        if (text.find(kw) != std::string::npos) return true;
    }
    return false;
}

} // namespace

domain::RetrievalDecision RetrievalRouter::classifyQuery(std::string_view query, const std::vector<std::string>& document_ids) {
    std::string q_lower = toLower(query);

    const auto now = std::chrono::system_clock::now();
    std::string now_str = std::format("{:%Y-%m-%d %H:%M:%S}", now);

    domain::RetrievalDecision decision{
        .query = std::string(query),
        .route = domain::RetrievalRoute::METADATA_SUFFICIENT,
        .reason = "Query can be resolved directly from declared metadata headers.",
        .signals = {},
        .documents_considered = document_ids,
        .chosen_path = "metadata_first_index",
        .timestamp = now_str
    };

    // Deep analysis keywords
    const std::vector<std::string> deep_keywords = {
        "por que", "por quê", "porque", "como", "why", "how", "argumento", "argument",
        "evidência", "evidence", "contradição", "contradiction", "comparação", "comparison",
        "interpretação", "sintese", "síntese", "synthesis", "justificativa", "justification"
    };

    // Text indexing / snippet keywords
    const std::vector<std::string> text_keywords = {
        "seção", "secao", "section", "trecho", "trechos", "snippet", "parágrafo", "paragrafo",
        "ocorrência", "ocorrencia", "localizar texto", "frase", "quote", "citação", "citacao"
    };

    // Metadata signals
    const std::vector<std::string> meta_keywords = {
        "projeto", "project", "versão", "versao", "version", "autor", "author", "status",
        "proveniência", "proveniencia", "provenance", "origem", "origin", "conceito", "concept",
        "relação", "relacao", "relation", "id", "título", "titulo", "title", "qual é o", "quem"
    };

    if (containsAny(q_lower, deep_keywords)) {
        decision.route = domain::RetrievalRoute::DEEP_ANALYSIS_REQUIRED;
        decision.reason = "Query requests reasoning, deep arguments, comparative synthesis or causal explanation contained in document body.";
        decision.chosen_path = "deep_text_analysis_flow";
        decision.signals.push_back("causal_or_epistemic_deepening");
    } else if (containsAny(q_lower, text_keywords)) {
        decision.route = domain::RetrievalRoute::TEXT_INDEX_REQUIRED;
        decision.reason = "Query targets specific sections, phrases, quotations or occurrences requiring full-text indexing.";
        decision.chosen_path = "fts5_text_index";
        decision.signals.push_back("text_search_intent");
    } else {
        decision.route = domain::RetrievalRoute::METADATA_SUFFICIENT;
        decision.reason = "Query targets identity, project, version, relations or declared epistemic status resolvable without full text.";
        decision.chosen_path = "metadata_first_index";
        decision.signals.push_back("metadata_target_intent");
    }

    return decision;
}

} // namespace contextlab::retrieval
