#include <cassert>
#include <iostream>
#include <filesystem>
#include "contextlab/application/ContextLabService.hpp"

int main() {
    std::cout << "Running Selective Deepening Lifecycle Integration Test...\n";

    std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "contextlab_test_deepen";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    contextlab::core::AppConfig config;
    config.storage.root = temp_dir / "data";

    std::filesystem::path repo_root = std::filesystem::current_path();
    while (!std::filesystem::exists(repo_root / "docs" / "CONTEXTLAB-BOOTSTRAP-001-v0.1.0.md")) {
        if (!repo_root.has_parent_path() || repo_root.parent_path() == repo_root) break;
        repo_root = repo_root.parent_path();
    }

    contextlab::application::ContextLabService service(config, repo_root);
    auto init_res = service.initialize();
    assert(init_res.has_value());

    auto bootstrap_path = repo_root / "docs" / "CONTEXTLAB-BOOTSTRAP-001-v0.1.0.md";
    auto ing_res = service.ingestFile(bootstrap_path);
    assert(ing_res.has_value());

    // Step 1: Query 1 (Metadata sufficient)
    auto search1 = service.search("Qual é o projeto deste documento?", "auto");
    assert(search1.has_value());
    assert(search1->route == "METADATA_SUFFICIENT");
    assert(search1->text_analysis_performed == false);

    // Step 2: Query 2 (Deep analysis required)
    auto search2 = service.search("Por que o sistema não deve misturar metadados declarados e derivados?", "auto");
    assert(search2.has_value());
    assert(search2->route == "DEEP_ANALYSIS_REQUIRED");

    // Step 3: Trigger deepen
    auto deepen_res = service.deepen("CONTEXTLAB-BOOTSTRAP-001");
    assert(deepen_res.has_value());
    assert(deepen_res->character_count > 0);
    assert(deepen_res->section_count > 0);

    // Verify document state is now text_analyzed = true
    auto doc_after = service.getDocument("CONTEXTLAB-BOOTSTRAP-001");
    assert(doc_after.has_value());
    assert(doc_after->text_analyzed == true);

    // Step 4: Perform FTS5 search
    auto fts_search = service.search("mandato", "text");
    assert(fts_search.has_value());
    assert(fts_search->text_analysis_performed == true);
    assert(!fts_search->results.empty());

    std::cout << "✓ Selective Deepening Lifecycle Test PASS!\n";

    std::filesystem::remove_all(temp_dir);
    return 0;
}
