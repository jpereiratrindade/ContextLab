#include "contextlab/persistence/Repository.hpp"
#include <format>

namespace contextlab::persistence {

core::Result<void> Repository::saveDocument(const domain::Document& doc) {
    auto stmt_res = db_.prepare(R"(
        INSERT INTO documents (
            id, title, subtitle, version, date_created, date_modified, language,
            document_type, lifecycle_state, publication_state, primary_project,
            resource_scope, self_describing, self_consumption_required,
            epistemic_status, primary_authority, text_analyzed, created_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            title = excluded.title,
            subtitle = excluded.subtitle,
            version = excluded.version,
            date_modified = excluded.date_modified,
            lifecycle_state = excluded.lifecycle_state,
            publication_state = excluded.publication_state,
            primary_project = excluded.primary_project,
            resource_scope = excluded.resource_scope,
            epistemic_status = excluded.epistemic_status,
            primary_authority = excluded.primary_authority,
            text_analyzed = excluded.text_analyzed;
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare saveDocument query");
    auto& stmt = *stmt_res;

    stmt.bindText(1, doc.id);
    stmt.bindText(2, doc.title);
    stmt.bindText(3, doc.subtitle);
    stmt.bindText(4, doc.version);
    stmt.bindText(5, doc.date_created);
    stmt.bindText(6, doc.date_modified);
    stmt.bindText(7, doc.language);
    stmt.bindText(8, doc.document_type);
    stmt.bindText(9, doc.lifecycle_state);
    stmt.bindText(10, doc.publication_state);
    stmt.bindText(11, doc.primary_project);
    stmt.bindText(12, doc.resource_scope);
    stmt.bindInt64(13, doc.self_describing ? 1 : 0);
    stmt.bindInt64(14, doc.self_consumption_required ? 1 : 0);
    stmt.bindText(15, doc.epistemic_status);
    stmt.bindText(16, domain::authorityToString(doc.primary_authority));
    stmt.bindInt64(17, doc.text_analyzed ? 1 : 0);
    stmt.bindText(18, doc.created_at);

    stmt.step();

    // Insert version history
    auto ver_stmt_res = db_.prepare(R"(
        INSERT OR IGNORE INTO document_versions (document_id, version, created_at)
        VALUES (?, ?, ?);
    )");
    if (ver_stmt_res) {
        ver_stmt_res->bindText(1, doc.id);
        ver_stmt_res->bindText(2, doc.version);
        ver_stmt_res->bindText(3, doc.created_at);
        ver_stmt_res->step();
    }

    return core::makeOk();
}

core::Result<void> Repository::updateDocument(const domain::Document& doc) {
    return saveDocument(doc);
}

core::Result<void> Repository::deleteDocument(const std::string& doc_id) {
    (void)db_.execute("DELETE FROM document_artifacts WHERE document_id = '" + doc_id + "';");
    (void)db_.execute("DELETE FROM metadata_envelopes WHERE document_id = '" + doc_id + "';");
    (void)db_.execute("DELETE FROM document_projects WHERE document_id = '" + doc_id + "';");
    (void)db_.execute("DELETE FROM document_concepts WHERE document_id = '" + doc_id + "';");
    (void)db_.execute("DELETE FROM relations WHERE document_id = '" + doc_id + "';");
    (void)db_.execute("DELETE FROM text_analysis WHERE document_id = '" + doc_id + "';");
    (void)db_.execute("DELETE FROM document_versions WHERE document_id = '" + doc_id + "';");
    (void)db_.execute("DELETE FROM document_text_fts WHERE document_id = '" + doc_id + "';");

    auto stmt_res = db_.prepare("DELETE FROM documents WHERE id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare deleteDocument");
    stmt_res->bindText(1, doc_id);
    (void)stmt_res->step();
    return core::makeOk();
}

core::Result<std::optional<domain::Document>> Repository::getDocument(const std::string& doc_id) {
    auto stmt_res = db_.prepare(R"(
        SELECT id, title, subtitle, version, date_created, date_modified, language,
               document_type, lifecycle_state, publication_state, primary_project,
               resource_scope, self_describing, self_consumption_required,
               epistemic_status, primary_authority, text_analyzed, created_at
        FROM documents WHERE id = ?;
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare getDocument query");
    auto& stmt = *stmt_res;
    stmt.bindText(1, doc_id);

    if (stmt.step()) {
        domain::Document doc{
            .id = stmt.getText(0),
            .title = stmt.getText(1),
            .subtitle = stmt.getText(2),
            .version = stmt.getText(3),
            .date_created = stmt.getText(4),
            .date_modified = stmt.getText(5),
            .language = stmt.getText(6),
            .document_type = stmt.getText(7),
            .lifecycle_state = stmt.getText(8),
            .publication_state = stmt.getText(9),
            .primary_project = stmt.getText(10),
            .resource_scope = stmt.getText(11),
            .self_describing = stmt.getInt64(12) != 0,
            .self_consumption_required = stmt.getInt64(13) != 0,
            .epistemic_status = stmt.getText(14),
            .primary_authority = domain::stringToAuthority(stmt.getText(15)),
            .text_analyzed = stmt.getInt64(16) != 0,
            .created_at = stmt.getText(17)
        };
        return core::makeOk(std::make_optional(std::move(doc)));
    }
    return core::makeOk(std::optional<domain::Document>{std::nullopt});
}

core::Result<std::vector<domain::Document>> Repository::getAllDocuments() {
    auto stmt_res = db_.prepare(R"(
        SELECT id, title, subtitle, version, date_created, date_modified, language,
               document_type, lifecycle_state, publication_state, primary_project,
               resource_scope, self_describing, self_consumption_required,
               epistemic_status, primary_authority, text_analyzed, created_at
        FROM documents ORDER BY created_at DESC;
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare getAllDocuments query");
    auto& stmt = *stmt_res;

    std::vector<domain::Document> docs;
    while (stmt.step()) {
        docs.push_back(domain::Document{
            .id = stmt.getText(0),
            .title = stmt.getText(1),
            .subtitle = stmt.getText(2),
            .version = stmt.getText(3),
            .date_created = stmt.getText(4),
            .date_modified = stmt.getText(5),
            .language = stmt.getText(6),
            .document_type = stmt.getText(7),
            .lifecycle_state = stmt.getText(8),
            .publication_state = stmt.getText(9),
            .primary_project = stmt.getText(10),
            .resource_scope = stmt.getText(11),
            .self_describing = stmt.getInt64(12) != 0,
            .self_consumption_required = stmt.getInt64(13) != 0,
            .epistemic_status = stmt.getText(14),
            .primary_authority = domain::stringToAuthority(stmt.getText(15)),
            .text_analyzed = stmt.getInt64(16) != 0,
            .created_at = stmt.getText(17)
        });
    }
    return core::makeOk(std::move(docs));
}

core::Result<size_t> Repository::countDocuments() {
    auto stmt_res = db_.prepare("SELECT COUNT(*) FROM documents;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to count documents");
    if (stmt_res->step()) {
        return core::makeOk(static_cast<size_t>(stmt_res->getInt64(0)));
    }
    return core::makeOk<size_t>(0);
}

core::Result<void> Repository::updateDocumentTextAnalyzed(const std::string& doc_id, bool analyzed) {
    auto stmt_res = db_.prepare("UPDATE documents SET text_analyzed = ? WHERE id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to update text_analyzed status");
    stmt_res->bindInt64(1, analyzed ? 1 : 0);
    stmt_res->bindText(2, doc_id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::saveArtifact(const domain::Artifact& artifact) {
    auto stmt_res = db_.prepare(R"(
        INSERT OR REPLACE INTO artifacts (sha256, original_filename, media_type, size_bytes, ingested_at, object_path)
        VALUES (?, ?, ?, ?, ?, ?);
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare saveArtifact");
    auto& stmt = *stmt_res;
    stmt.bindText(1, artifact.sha256);
    stmt.bindText(2, artifact.original_filename);
    stmt.bindText(3, artifact.media_type);
    stmt.bindInt64(4, static_cast<int64_t>(artifact.size_bytes));
    stmt.bindText(5, artifact.ingested_at);
    stmt.bindText(6, artifact.object_path);
    stmt.step();
    return core::makeOk();
}

core::Result<std::optional<domain::Artifact>> Repository::getArtifact(const std::string& sha256) {
    auto stmt_res = db_.prepare("SELECT sha256, original_filename, media_type, size_bytes, ingested_at, object_path FROM artifacts WHERE sha256 = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare getArtifact");
    stmt_res->bindText(1, sha256);
    if (stmt_res->step()) {
        return core::makeOk(std::make_optional(domain::Artifact{
            .sha256 = stmt_res->getText(0),
            .original_filename = stmt_res->getText(1),
            .media_type = stmt_res->getText(2),
            .size_bytes = static_cast<uint64_t>(stmt_res->getInt64(3)),
            .ingested_at = stmt_res->getText(4),
            .object_path = stmt_res->getText(5)
        }));
    }
    return core::makeOk(std::optional<domain::Artifact>{std::nullopt});
}

core::Result<std::vector<domain::Artifact>> Repository::getAllArtifacts() {
    auto stmt_res = db_.prepare("SELECT sha256, original_filename, media_type, size_bytes, ingested_at, object_path FROM artifacts ORDER BY ingested_at DESC;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare getAllArtifacts");
    std::vector<domain::Artifact> artifacts;
    while (stmt_res->step()) {
        artifacts.push_back(domain::Artifact{
            .sha256 = stmt_res->getText(0),
            .original_filename = stmt_res->getText(1),
            .media_type = stmt_res->getText(2),
            .size_bytes = static_cast<uint64_t>(stmt_res->getInt64(3)),
            .ingested_at = stmt_res->getText(4),
            .object_path = stmt_res->getText(5)
        });
    }
    return core::makeOk(std::move(artifacts));
}

core::Result<void> Repository::linkDocumentArtifact(const std::string& doc_id, const std::string& sha256, const std::string& role) {
    auto stmt_res = db_.prepare(R"(
        INSERT OR REPLACE INTO document_artifacts (document_id, artifact_sha256, role, linked_at)
        VALUES (?, ?, ?, datetime('now'));
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to link document artifact");
    stmt_res->bindText(1, doc_id);
    stmt_res->bindText(2, sha256);
    stmt_res->bindText(3, role);
    stmt_res->step();
    return core::makeOk();
}

core::Result<std::vector<domain::Artifact>> Repository::getArtifactsForDocument(const std::string& doc_id) {
    auto stmt_res = db_.prepare(R"(
        SELECT a.sha256, a.original_filename, a.media_type, a.size_bytes, a.ingested_at, a.object_path
        FROM artifacts a
        JOIN document_artifacts da ON a.sha256 = da.artifact_sha256
        WHERE da.document_id = ?;
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query document artifacts");
    stmt_res->bindText(1, doc_id);
    std::vector<domain::Artifact> artifacts;
    while (stmt_res->step()) {
        artifacts.push_back(domain::Artifact{
            .sha256 = stmt_res->getText(0),
            .original_filename = stmt_res->getText(1),
            .media_type = stmt_res->getText(2),
            .size_bytes = static_cast<uint64_t>(stmt_res->getInt64(3)),
            .ingested_at = stmt_res->getText(4),
            .object_path = stmt_res->getText(5)
        });
    }
    return core::makeOk(std::move(artifacts));
}

core::Result<void> Repository::saveMetadataEnvelope(const domain::MetadataEnvelope& env) {
    auto stmt_res = db_.prepare(R"(
        INSERT OR REPLACE INTO metadata_envelopes (
            id, document_id, authority, schema_id, schema_version, payload_json,
            producer, method, confidence, source_artifact_sha256, created_at,
            validation_state, validation_error
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare saveMetadataEnvelope");
    auto& stmt = *stmt_res;
    stmt.bindText(1, env.id);
    stmt.bindText(2, env.document_id);
    stmt.bindText(3, domain::authorityToString(env.authority));
    stmt.bindText(4, env.schema_id);
    stmt.bindText(5, env.schema_version);
    stmt.bindText(6, env.payload.dump());
    stmt.bindText(7, env.producer);
    stmt.bindText(8, env.method);
    stmt.bindDouble(9, env.confidence);
    stmt.bindText(10, env.source_artifact_sha256);
    stmt.bindText(11, env.created_at);
    stmt.bindText(12, domain::validationStateToString(env.validation_state));
    stmt.bindText(13, env.validation_error);
    stmt.step();
    return core::makeOk();
}

core::Result<std::vector<domain::MetadataEnvelope>> Repository::getMetadataEnvelopes(const std::string& doc_id) {
    auto stmt_res = db_.prepare(R"(
        SELECT id, document_id, authority, schema_id, schema_version, payload_json,
               producer, method, confidence, source_artifact_sha256, created_at,
               validation_state, validation_error
        FROM metadata_envelopes WHERE document_id = ? ORDER BY created_at ASC;
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query metadata envelopes");
    stmt_res->bindText(1, doc_id);
    std::vector<domain::MetadataEnvelope> envelopes;
    while (stmt_res->step()) {
        nlohmann::json payload = nlohmann::json::object();
        try {
            payload = nlohmann::json::parse(stmt_res->getText(5));
        } catch (...) {}

        envelopes.push_back(domain::MetadataEnvelope{
            .id = stmt_res->getText(0),
            .document_id = stmt_res->getText(1),
            .authority = domain::stringToAuthority(stmt_res->getText(2)),
            .schema_id = stmt_res->getText(3),
            .schema_version = stmt_res->getText(4),
            .payload = std::move(payload),
            .producer = stmt_res->getText(6),
            .method = stmt_res->getText(7),
            .confidence = stmt_res->getDouble(8),
            .source_artifact_sha256 = stmt_res->getText(9),
            .created_at = stmt_res->getText(10),
            .validation_state = domain::stringToValidationState(stmt_res->getText(11)),
            .validation_error = stmt_res->getText(12)
        });
    }
    return core::makeOk(std::move(envelopes));
}

core::Result<std::vector<domain::MetadataEnvelope>> Repository::getAllMetadataEnvelopes() {
    auto stmt_res = db_.prepare(R"(
        SELECT id, document_id, authority, schema_id, schema_version, payload_json,
               producer, method, confidence, source_artifact_sha256, created_at,
               validation_state, validation_error
        FROM metadata_envelopes ORDER BY created_at DESC;
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query all metadata envelopes");
    std::vector<domain::MetadataEnvelope> envelopes;
    while (stmt_res->step()) {
        nlohmann::json payload = nlohmann::json::object();
        try {
            payload = nlohmann::json::parse(stmt_res->getText(5));
        } catch (...) {}

        envelopes.push_back(domain::MetadataEnvelope{
            .id = stmt_res->getText(0),
            .document_id = stmt_res->getText(1),
            .authority = domain::stringToAuthority(stmt_res->getText(2)),
            .schema_id = stmt_res->getText(3),
            .schema_version = stmt_res->getText(4),
            .payload = std::move(payload),
            .producer = stmt_res->getText(6),
            .method = stmt_res->getText(7),
            .confidence = stmt_res->getDouble(8),
            .source_artifact_sha256 = stmt_res->getText(9),
            .created_at = stmt_res->getText(10),
            .validation_state = domain::stringToValidationState(stmt_res->getText(11)),
            .validation_error = stmt_res->getText(12)
        });
    }
    return core::makeOk(std::move(envelopes));
}

core::Result<void> Repository::saveProject(const domain::Project& project) {
    auto stmt_res = db_.prepare(R"(
        INSERT OR REPLACE INTO projects (
            id, name, kind, research_domain_json, stage, object_of_study, central_question, engineering_question
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare saveProject");
    auto& stmt = *stmt_res;
    stmt.bindText(1, project.id);
    stmt.bindText(2, project.name);
    stmt.bindText(3, project.kind);
    stmt.bindText(4, nlohmann::json(project.research_domain).dump());
    stmt.bindText(5, project.stage);
    stmt.bindText(6, project.object_of_study);
    stmt.bindText(7, project.central_question);
    stmt.bindText(8, project.engineering_question);
    stmt.step();
    return core::makeOk();
}

core::Result<std::optional<domain::Project>> Repository::getProject(const std::string& project_id) {
    auto stmt_res = db_.prepare(R"(
        SELECT id, name, kind, research_domain_json, stage, object_of_study, central_question, engineering_question
        FROM projects WHERE id = ?;
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare getProject");
    stmt_res->bindText(1, project_id);
    if (stmt_res->step()) {
        std::vector<std::string> domains;
        try {
            auto j = nlohmann::json::parse(stmt_res->getText(3));
            if (j.is_array()) domains = j.get<std::vector<std::string>>();
        } catch (...) {}

        return core::makeOk(std::make_optional(domain::Project{
            .id = stmt_res->getText(0),
            .name = stmt_res->getText(1),
            .kind = stmt_res->getText(2),
            .research_domain = std::move(domains),
            .stage = stmt_res->getText(4),
            .object_of_study = stmt_res->getText(5),
            .central_question = stmt_res->getText(6),
            .engineering_question = stmt_res->getText(7)
        }));
    }
    return core::makeOk(std::optional<domain::Project>{std::nullopt});
}

core::Result<std::vector<domain::Project>> Repository::getAllProjects() {
    auto stmt_res = db_.prepare("SELECT id, name, kind, research_domain_json, stage, object_of_study, central_question, engineering_question FROM projects;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query projects");
    std::vector<domain::Project> projects;
    while (stmt_res->step()) {
        std::vector<std::string> domains;
        try {
            auto j = nlohmann::json::parse(stmt_res->getText(3));
            if (j.is_array()) domains = j.get<std::vector<std::string>>();
        } catch (...) {}

        projects.push_back(domain::Project{
            .id = stmt_res->getText(0),
            .name = stmt_res->getText(1),
            .kind = stmt_res->getText(2),
            .research_domain = std::move(domains),
            .stage = stmt_res->getText(4),
            .object_of_study = stmt_res->getText(5),
            .central_question = stmt_res->getText(6),
            .engineering_question = stmt_res->getText(7)
        });
    }
    return core::makeOk(std::move(projects));
}

core::Result<void> Repository::linkDocumentProject(const std::string& doc_id, const std::string& project_id) {
    auto stmt_res = db_.prepare("INSERT OR REPLACE INTO document_projects (document_id, project_id) VALUES (?, ?);");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to link document project");
    stmt_res->bindText(1, doc_id);
    stmt_res->bindText(2, project_id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::saveConcept(const domain::Concept& concept_item) {
    auto stmt_res = db_.prepare("INSERT OR REPLACE INTO concepts (id, label, role) VALUES (?, ?, ?);");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to save concept");
    stmt_res->bindText(1, concept_item.id);
    stmt_res->bindText(2, concept_item.label);
    stmt_res->bindText(3, concept_item.role);
    stmt_res->step();
    return core::makeOk();
}

core::Result<std::vector<domain::Concept>> Repository::getConceptsForDocument(const std::string& doc_id) {
    auto stmt_res = db_.prepare(R"(
        SELECT c.id, c.label, c.role
        FROM concepts c
        JOIN document_concepts dc ON c.id = dc.concept_id
        WHERE dc.document_id = ?;
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query concepts");
    stmt_res->bindText(1, doc_id);
    std::vector<domain::Concept> concepts;
    while (stmt_res->step()) {
        concepts.push_back(domain::Concept{
            .id = stmt_res->getText(0),
            .label = stmt_res->getText(1),
            .role = stmt_res->getText(2)
        });
    }
    return core::makeOk(std::move(concepts));
}

core::Result<void> Repository::linkDocumentConcept(const std::string& doc_id, const std::string& concept_id) {
    auto stmt_res = db_.prepare("INSERT OR REPLACE INTO document_concepts (document_id, concept_id) VALUES (?, ?);");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to link document concept");
    stmt_res->bindText(1, doc_id);
    stmt_res->bindText(2, concept_id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::deleteMetadataEnvelope(const std::string& id) {
    auto stmt_res = db_.prepare("DELETE FROM metadata_envelopes WHERE id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare deleteMetadataEnvelope");
    stmt_res->bindText(1, id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::deleteProject(const std::string& project_id) {
    (void)db_.execute("DELETE FROM document_projects WHERE project_id = '" + project_id + "';");
    auto stmt_res = db_.prepare("DELETE FROM projects WHERE id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare deleteProject");
    stmt_res->bindText(1, project_id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::unlinkDocumentProject(const std::string& doc_id, const std::string& project_id) {
    auto stmt_res = db_.prepare("DELETE FROM document_projects WHERE document_id = ? AND project_id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare unlinkDocumentProject");
    stmt_res->bindText(1, doc_id);
    stmt_res->bindText(2, project_id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::deleteConcept(const std::string& concept_id) {
    (void)db_.execute("DELETE FROM document_concepts WHERE concept_id = '" + concept_id + "';");
    auto stmt_res = db_.prepare("DELETE FROM concepts WHERE id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare deleteConcept");
    stmt_res->bindText(1, concept_id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::unlinkDocumentConcept(const std::string& doc_id, const std::string& concept_id) {
    auto stmt_res = db_.prepare("DELETE FROM document_concepts WHERE document_id = ? AND concept_id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare unlinkDocumentConcept");
    stmt_res->bindText(1, doc_id);
    stmt_res->bindText(2, concept_id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::saveRelation(const domain::Relation& relation) {
    auto stmt_res = db_.prepare("INSERT INTO relations (subject, predicate, object, document_id) VALUES (?, ?, ?, ?);");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to save relation");
    stmt_res->bindText(1, relation.subject);
    stmt_res->bindText(2, relation.predicate);
    stmt_res->bindText(3, relation.object);
    stmt_res->bindText(4, relation.document_id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::updateRelation(const domain::Relation& relation) {
    auto stmt_res = db_.prepare("UPDATE relations SET subject = ?, predicate = ?, object = ?, document_id = ? WHERE id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare updateRelation");
    stmt_res->bindText(1, relation.subject);
    stmt_res->bindText(2, relation.predicate);
    stmt_res->bindText(3, relation.object);
    stmt_res->bindText(4, relation.document_id);
    stmt_res->bindInt64(5, relation.id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<void> Repository::deleteRelation(int64_t relation_id) {
    auto stmt_res = db_.prepare("DELETE FROM relations WHERE id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare deleteRelation");
    stmt_res->bindInt64(1, relation_id);
    stmt_res->step();
    return core::makeOk();
}

core::Result<std::vector<domain::Relation>> Repository::getRelationsForDocument(const std::string& doc_id) {
    auto stmt_res = db_.prepare("SELECT id, subject, predicate, object, document_id FROM relations WHERE document_id = ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query document relations");
    stmt_res->bindText(1, doc_id);
    std::vector<domain::Relation> relations;
    while (stmt_res->step()) {
        relations.push_back(domain::Relation{
            .id = stmt_res->getInt64(0),
            .subject = stmt_res->getText(1),
            .predicate = stmt_res->getText(2),
            .object = stmt_res->getText(3),
            .document_id = stmt_res->getText(4)
        });
    }
    return core::makeOk(std::move(relations));
}

core::Result<std::vector<domain::Relation>> Repository::getAllRelations() {
    auto stmt_res = db_.prepare("SELECT id, subject, predicate, object, document_id FROM relations ORDER BY id ASC;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query all relations");
    std::vector<domain::Relation> relations;
    while (stmt_res->step()) {
        relations.push_back(domain::Relation{
            .id = stmt_res->getInt64(0),
            .subject = stmt_res->getText(1),
            .predicate = stmt_res->getText(2),
            .object = stmt_res->getText(3),
            .document_id = stmt_res->getText(4)
        });
    }
    return core::makeOk(std::move(relations));
}

core::Result<int64_t> Repository::recordIngestionEvent(const domain::IngestionEvent& ev) {
    auto stmt_res = db_.prepare(R"(
        INSERT INTO ingestion_events (timestamp, source_file, artifact_sha256, document_id, status, message, details_json)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to record ingestion event");
    auto& stmt = *stmt_res;
    stmt.bindText(1, ev.timestamp);
    stmt.bindText(2, ev.source_file);
    stmt.bindText(3, ev.artifact_sha256);
    stmt.bindText(4, ev.document_id);
    stmt.bindText(5, ev.status);
    stmt.bindText(6, ev.message);
    stmt.bindText(7, ev.details.dump());
    stmt.step();
    return core::makeOk(db_.lastInsertRowId());
}

core::Result<std::vector<domain::IngestionEvent>> Repository::getRecentIngestionEvents(int limit) {
    auto stmt_res = db_.prepare("SELECT id, timestamp, source_file, artifact_sha256, document_id, status, message, details_json FROM ingestion_events ORDER BY id DESC LIMIT ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query ingestion events");
    stmt_res->bindInt64(1, limit);
    std::vector<domain::IngestionEvent> events;
    while (stmt_res->step()) {
        nlohmann::json details = nlohmann::json::object();
        try {
            details = nlohmann::json::parse(stmt_res->getText(7));
        } catch (...) {}

        events.push_back(domain::IngestionEvent{
            .id = stmt_res->getInt64(0),
            .timestamp = stmt_res->getText(1),
            .source_file = stmt_res->getText(2),
            .artifact_sha256 = stmt_res->getText(3),
            .document_id = stmt_res->getText(4),
            .status = stmt_res->getText(5),
            .message = stmt_res->getText(6),
            .details = std::move(details)
        });
    }
    return core::makeOk(std::move(events));
}

core::Result<int64_t> Repository::recordRetrievalDecision(const domain::RetrievalDecision& dec) {
    auto stmt_res = db_.prepare(R"(
        INSERT INTO retrieval_decisions (query, route, reason, signals_json, documents_considered_json, chosen_path, timestamp)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to record retrieval decision");
    auto& stmt = *stmt_res;
    stmt.bindText(1, dec.query);
    stmt.bindText(2, domain::retrievalRouteToString(dec.route));
    stmt.bindText(3, dec.reason);
    stmt.bindText(4, nlohmann::json(dec.signals).dump());
    stmt.bindText(5, nlohmann::json(dec.documents_considered).dump());
    stmt.bindText(6, dec.chosen_path);
    stmt.bindText(7, dec.timestamp);
    stmt.step();
    return core::makeOk(db_.lastInsertRowId());
}

core::Result<std::vector<domain::RetrievalDecision>> Repository::getRecentRetrievalDecisions(int limit) {
    auto stmt_res = db_.prepare("SELECT id, query, route, reason, signals_json, documents_considered_json, chosen_path, timestamp FROM retrieval_decisions ORDER BY id DESC LIMIT ?;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query retrieval decisions");
    stmt_res->bindInt64(1, limit);
    std::vector<domain::RetrievalDecision> decisions;
    while (stmt_res->step()) {
        std::vector<std::string> signals;
        std::vector<std::string> docs;
        try {
            auto s_j = nlohmann::json::parse(stmt_res->getText(4));
            if (s_j.is_array()) signals = s_j.get<std::vector<std::string>>();
        } catch (...) {}
        try {
            auto d_j = nlohmann::json::parse(stmt_res->getText(5));
            if (d_j.is_array()) docs = d_j.get<std::vector<std::string>>();
        } catch (...) {}

        decisions.push_back(domain::RetrievalDecision{
            .id = stmt_res->getInt64(0),
            .query = stmt_res->getText(1),
            .route = domain::RetrievalRoute::METADATA_SUFFICIENT, // will be matched below
            .reason = stmt_res->getText(3),
            .signals = std::move(signals),
            .documents_considered = std::move(docs),
            .chosen_path = stmt_res->getText(6),
            .timestamp = stmt_res->getText(7)
        });
        std::string r_str = stmt_res->getText(2);
        if (r_str == "TEXT_INDEX_REQUIRED") decisions.back().route = domain::RetrievalRoute::TEXT_INDEX_REQUIRED;
        else if (r_str == "DEEP_ANALYSIS_REQUIRED") decisions.back().route = domain::RetrievalRoute::DEEP_ANALYSIS_REQUIRED;
        else if (r_str == "UNRESOLVED") decisions.back().route = domain::RetrievalRoute::UNRESOLVED;
    }
    return core::makeOk(std::move(decisions));
}

core::Result<void> Repository::saveTextAnalysis(const domain::TextAnalysis& analysis) {
    auto stmt_res = db_.prepare(R"(
        INSERT OR REPLACE INTO text_analyses (
            document_id, analyzed_at, character_count, word_count, line_count, section_count,
            top_terms_json, sections_json, extraction_method, source_digest, sample_preview
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to save text analysis");
    auto& stmt = *stmt_res;
    stmt.bindText(1, analysis.document_id);
    stmt.bindText(2, analysis.analyzed_at);
    stmt.bindInt64(3, static_cast<int64_t>(analysis.character_count));
    stmt.bindInt64(4, static_cast<int64_t>(analysis.word_count));
    stmt.bindInt64(5, static_cast<int64_t>(analysis.line_count));
    stmt.bindInt64(6, static_cast<int64_t>(analysis.section_count));
    stmt.bindText(7, nlohmann::json(analysis.top_terms).dump());
    stmt.bindText(8, nlohmann::json(analysis.sections).dump());
    stmt.bindText(9, analysis.extraction_method);
    stmt.bindText(10, analysis.source_digest);
    stmt.bindText(11, analysis.sample_preview);
    stmt.step();

    updateDocumentTextAnalyzed(analysis.document_id, true);
    return core::makeOk();
}

core::Result<std::optional<domain::TextAnalysis>> Repository::getTextAnalysis(const std::string& doc_id) {
    auto stmt_res = db_.prepare(R"(
        SELECT id, document_id, analyzed_at, character_count, word_count, line_count, section_count,
               top_terms_json, sections_json, extraction_method, source_digest, sample_preview
        FROM text_analyses WHERE document_id = ?;
    )");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to get text analysis");
    stmt_res->bindText(1, doc_id);
    if (stmt_res->step()) {
        std::vector<std::string> terms;
        std::vector<std::string> sections;
        try {
            auto t_j = nlohmann::json::parse(stmt_res->getText(7));
            if (t_j.is_array()) terms = t_j.get<std::vector<std::string>>();
        } catch (...) {}
        try {
            auto s_j = nlohmann::json::parse(stmt_res->getText(8));
            if (s_j.is_array()) sections = s_j.get<std::vector<std::string>>();
        } catch (...) {}

        return core::makeOk(std::make_optional(domain::TextAnalysis{
            .id = stmt_res->getInt64(0),
            .document_id = stmt_res->getText(1),
            .analyzed_at = stmt_res->getText(2),
            .character_count = static_cast<uint64_t>(stmt_res->getInt64(3)),
            .word_count = static_cast<uint64_t>(stmt_res->getInt64(4)),
            .line_count = static_cast<uint64_t>(stmt_res->getInt64(5)),
            .section_count = static_cast<uint64_t>(stmt_res->getInt64(6)),
            .top_terms = std::move(terms),
            .sections = std::move(sections),
            .extraction_method = stmt_res->getText(9),
            .source_digest = stmt_res->getText(10),
            .sample_preview = stmt_res->getText(11)
        }));
    }
    return core::makeOk(std::optional<domain::TextAnalysis>{std::nullopt});
}

core::Result<void> Repository::indexDocumentText(const std::string& doc_id, const std::string& title, const std::string& body_text) {
    // Remove existing if any
    auto del_res = db_.prepare("DELETE FROM document_text_fts WHERE document_id = ?;");
    if (del_res) {
        del_res->bindText(1, doc_id);
        del_res->step();
    }

    auto ins_res = db_.prepare("INSERT INTO document_text_fts (document_id, title, body_text) VALUES (?, ?, ?);");
    if (!ins_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to insert into FTS5 index");
    ins_res->bindText(1, doc_id);
    ins_res->bindText(2, title);
    ins_res->bindText(3, body_text);
    ins_res->step();
    return core::makeOk();
}

core::Result<std::vector<std::pair<std::string, std::string>>> Repository::searchFts(const std::string& term) {
    auto stmt_res = db_.prepare(R"(
        SELECT document_id, snippet(document_text_fts, 2, '<b>', '</b>', '...', 20)
        FROM document_text_fts
        WHERE document_text_fts MATCH ?;
    )");
    if (!stmt_res) {
        return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to prepare FTS5 search query");
    }
    stmt_res->bindText(1, term);
    std::vector<std::pair<std::string, std::string>> results;
    while (stmt_res->step()) {
        results.emplace_back(stmt_res->getText(0), stmt_res->getText(1));
    }
    return core::makeOk(std::move(results));
}

} // namespace contextlab::persistence
