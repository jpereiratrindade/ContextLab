#include <cassert>
#include <iostream>
#include <filesystem>
#include "contextlab/application/ContextLabService.hpp"

int main() {
    std::cout << "Running CL-EXP-000 Self-Ingestion Experiment Witness...\n";

    // Setup temporary test database & environment
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "contextlab_test_exp000";
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

    // 1. Initialize
    auto init_res = service.initialize();
    assert(init_res.has_value());

    // 2. Ingest bootstrap document
    auto bootstrap_path = repo_root / "docs" / "CONTEXTLAB-BOOTSTRAP-001-v0.1.0.md";
    assert(std::filesystem::exists(bootstrap_path));

    auto ingest_res = service.ingestFile(bootstrap_path);
    assert(ingest_res.has_value());
    assert(ingest_res->success == true);
    assert(ingest_res->document_id == "CONTEXTLAB-BOOTSTRAP-001");
    assert(ingest_res->has_declared_context == true);
    assert(ingest_res->validation_state == contextlab::domain::ValidationState::VALID);
    assert(ingest_res->text_analysis_performed == false);

    // 3. Query document and verify invariant witnesses
    auto doc_res = service.getDocument("CONTEXTLAB-BOOTSTRAP-001");
    assert(doc_res.has_value());

    const auto& doc = *doc_res;
    assert(doc.id == "CONTEXTLAB-BOOTSTRAP-001");
    assert(doc.primary_project == "ContextLab");
    assert(doc.primary_authority == contextlab::domain::Authority::DECLARED);
    assert(doc.self_consumption_required == true);
    assert(doc.text_analyzed == false); // Full text is NOT analyzed yet

    std::cout << "CL-EXP-000 Witness: PASS (Document organized purely from declared metadata without reading body text)\n";

    std::filesystem::remove_all(temp_dir);
    return 0;
}
