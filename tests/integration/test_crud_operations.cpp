#include <cassert>
#include <iostream>
#include <filesystem>
#include "contextlab/application/ContextLabService.hpp"

int main() {
    std::cout << "Running CRUD Operations Integration Test...\n";

    std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "contextlab_test_crud";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    contextlab::core::AppConfig config;
    config.storage.root = temp_dir / "data";

    std::filesystem::path repo_root = std::filesystem::current_path();
    while (!std::filesystem::exists(repo_root / "schemas")) {
        if (!repo_root.has_parent_path() || repo_root.parent_path() == repo_root) break;
        repo_root = repo_root.parent_path();
    }

    contextlab::application::ContextLabService service(config, repo_root);
    auto init_res = service.initialize();
    assert(init_res.has_value());

    // 1. Ingest a document
    std::string sample_md = R"(# Test Research Note
```context-metadata+json
{
  "schema": {
    "id": "urn:contextlab:document-context",
    "version": "0.1.0"
  },
  "document": {
    "id": "CRUD-DOC-001",
    "title": "Initial Title",
    "primary_project": "Project Alpha"
  },
  "relations": [
    {"subject": "CRUD-DOC-001", "predicate": "references", "object": "Concept-A"}
  ]
}
```
## Section One
Content body of the research document.
)";

    auto ingest_res = service.ingestContent(sample_md, "crud_test.md", "text/markdown");
    assert(ingest_res.has_value());
    assert(ingest_res->document_id == "CRUD-DOC-001");

    // 2. Read Document
    auto doc_res = service.getDocument("CRUD-DOC-001");
    assert(doc_res.has_value());
    assert(doc_res->title == "Initial Title");
    assert(doc_res->primary_project == "Project Alpha");

    // 3. Update Document
    auto updated_doc = *doc_res;
    updated_doc.title = "Updated Title for CRUD";
    updated_doc.epistemic_status = "verified_hypothesis";
    auto update_res = service.updateDocument(updated_doc);
    assert(update_res.has_value());

    auto doc_after_update = service.getDocument("CRUD-DOC-001");
    assert(doc_after_update.has_value());
    assert(doc_after_update->title == "Updated Title for CRUD");
    assert(doc_after_update->epistemic_status == "verified_hypothesis");

    // 4. Project CRUD
    contextlab::domain::Project new_proj{
        .id = "Proj-Beta",
        .name = "Project Beta",
        .kind = "applied_research",
        .research_domain = {"Software Engineering", "Resilience"},
        .stage = "active",
        .object_of_study = "Autonomous Agents",
        .central_question = "How to automate verification?",
        .engineering_question = "How to build zero-friction tooling?"
    };
    auto create_proj_res = service.createProject(new_proj);
    assert(create_proj_res.has_value());

    auto projs_list = service.listProjects();
    assert(projs_list.has_value());
    bool found_beta = false;
    for (const auto& p : *projs_list) {
        if (p.id == "Proj-Beta") found_beta = true;
    }
    assert(found_beta);

    auto del_proj_res = service.deleteProject("Proj-Beta");
    assert(del_proj_res.has_value());

    // 5. Relation CRUD
    contextlab::domain::Relation new_rel{
        .id = 0,
        .subject = "CRUD-DOC-001",
        .predicate = "contributes_to",
        .object = "SisterEcosystem",
        .document_id = "CRUD-DOC-001"
    };
    auto create_rel_res = service.createRelation(new_rel);
    assert(create_rel_res.has_value());

    auto rels_list = service.listRelations();
    assert(rels_list.has_value());
    assert(!rels_list->empty());
    int64_t rel_to_del_id = (*rels_list)[0].id;

    auto del_rel_res = service.deleteRelation(rel_to_del_id);
    assert(del_rel_res.has_value());

    // 6. Delete Document
    auto del_doc_res = service.deleteDocument("CRUD-DOC-001");
    assert(del_doc_res.has_value());

    auto doc_after_delete = service.getDocument("CRUD-DOC-001");
    assert(!doc_after_delete.has_value() || !doc_after_delete->has_value());

    std::cout << "✓ CRUD Operations Integration Test PASS!\n";

    std::filesystem::remove_all(temp_dir);
    return 0;
}
