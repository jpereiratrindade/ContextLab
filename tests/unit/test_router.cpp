#include <cassert>
#include <iostream>
#include "contextlab/retrieval/RetrievalRouter.hpp"

void testRouterClassification() {
    using namespace contextlab::domain;
    using namespace contextlab::retrieval;

    // 1. Metadata sufficient query
    auto dec1 = RetrievalRouter::classifyQuery("qual o projeto do documento CONTEXTLAB-BOOTSTRAP-001");
    assert(dec1.route == RetrievalRoute::METADATA_SUFFICIENT);
    assert(dec1.chosen_path == "metadata_first_index");

    // 2. Text indexing query
    auto dec2 = RetrievalRouter::classifyQuery("localizar seção de arquitetura e trecho");
    assert(dec2.route == RetrievalRoute::TEXT_INDEX_REQUIRED);
    assert(dec2.chosen_path == "fts5_text_index");

    // 3. Deep analysis query
    auto dec3 = RetrievalRouter::classifyQuery("Por que o sistema não deve misturar metadados declarados e derivados?");
    assert(dec3.route == RetrievalRoute::DEEP_ANALYSIS_REQUIRED);
    assert(dec3.chosen_path == "deep_text_analysis_flow");

    std::cout << "✓ testRouterClassification passed\n";
}

int main() {
    std::cout << "Running Retrieval Router Unit Tests...\n";
    testRouterClassification();
    std::cout << "All Retrieval Router Unit Tests PASS!\n";
    return 0;
}
