#include "contextlab/persistence/Migrations.hpp"
#include <chrono>
#include <format>
#include <vector>

namespace contextlab::persistence {

core::Result<void> Migrations::applyAll(Database& db) {
    auto res = db.execute(R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            applied_at TEXT NOT NULL
        );
    )");
    if (!res) return res;

    // Check applied migrations
    auto stmt_res = db.prepare("SELECT version FROM schema_migrations;");
    if (!stmt_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to query schema migrations");
    auto& stmt = *stmt_res;

    std::vector<int> applied;
    while (stmt.step()) {
        applied.push_back(static_cast<int>(stmt.getInt64(0)));
    }

    auto is_applied = [&](int v) {
        for (int x : applied) if (x == v) return true;
        return false;
    };

    const auto now = std::chrono::system_clock::now();
    std::string now_str = std::format("{:%Y-%m-%d %H:%M:%S}", now);

    // Migration 1: Core baseline tables
    if (!is_applied(1)) {
        auto m1_res = db.execute(R"(
            CREATE TABLE IF NOT EXISTS documents (
                id TEXT PRIMARY KEY,
                title TEXT NOT NULL,
                subtitle TEXT,
                version TEXT NOT NULL,
                date_created TEXT,
                date_modified TEXT,
                language TEXT,
                document_type TEXT,
                lifecycle_state TEXT,
                publication_state TEXT,
                primary_project TEXT,
                resource_scope TEXT,
                self_describing INTEGER NOT NULL DEFAULT 1,
                self_consumption_required INTEGER NOT NULL DEFAULT 0,
                epistemic_status TEXT,
                primary_authority TEXT NOT NULL DEFAULT 'author_declared',
                text_analyzed INTEGER NOT NULL DEFAULT 0,
                created_at TEXT NOT NULL
            );

            CREATE TABLE IF NOT EXISTS document_versions (
                document_id TEXT NOT NULL,
                version TEXT NOT NULL,
                created_at TEXT NOT NULL,
                PRIMARY KEY (document_id, version),
                FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS artifacts (
                sha256 TEXT PRIMARY KEY,
                original_filename TEXT NOT NULL,
                media_type TEXT NOT NULL,
                size_bytes INTEGER NOT NULL,
                ingested_at TEXT NOT NULL,
                object_path TEXT NOT NULL
            );

            CREATE TABLE IF NOT EXISTS document_artifacts (
                document_id TEXT NOT NULL,
                artifact_sha256 TEXT NOT NULL,
                role TEXT NOT NULL DEFAULT 'primary_source',
                linked_at TEXT NOT NULL,
                PRIMARY KEY (document_id, artifact_sha256),
                FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE,
                FOREIGN KEY (artifact_sha256) REFERENCES artifacts(sha256) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS metadata_envelopes (
                id TEXT PRIMARY KEY,
                document_id TEXT NOT NULL,
                authority TEXT NOT NULL,
                schema_id TEXT NOT NULL,
                schema_version TEXT NOT NULL,
                payload_json TEXT NOT NULL,
                producer TEXT,
                method TEXT,
                confidence REAL NOT NULL DEFAULT 1.0,
                source_artifact_sha256 TEXT,
                created_at TEXT NOT NULL,
                validation_state TEXT NOT NULL DEFAULT 'UNVALIDATED',
                validation_error TEXT,
                FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS projects (
                id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                kind TEXT,
                research_domain_json TEXT,
                stage TEXT,
                object_of_study TEXT,
                central_question TEXT,
                engineering_question TEXT
            );

            CREATE TABLE IF NOT EXISTS document_projects (
                document_id TEXT NOT NULL,
                project_id TEXT NOT NULL,
                PRIMARY KEY (document_id, project_id),
                FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE,
                FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS concepts (
                id TEXT PRIMARY KEY,
                label TEXT NOT NULL,
                role TEXT
            );

            CREATE TABLE IF NOT EXISTS document_concepts (
                document_id TEXT NOT NULL,
                concept_id TEXT NOT NULL,
                PRIMARY KEY (document_id, concept_id),
                FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE,
                FOREIGN KEY (concept_id) REFERENCES concepts(id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS relations (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                subject TEXT NOT NULL,
                predicate TEXT NOT NULL,
                object TEXT NOT NULL,
                document_id TEXT NOT NULL,
                FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS ingestion_events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp TEXT NOT NULL,
                source_file TEXT NOT NULL,
                artifact_sha256 TEXT,
                document_id TEXT,
                status TEXT NOT NULL,
                message TEXT,
                details_json TEXT
            );

            CREATE TABLE IF NOT EXISTS text_analyses (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                document_id TEXT NOT NULL,
                analyzed_at TEXT NOT NULL,
                character_count INTEGER NOT NULL,
                word_count INTEGER NOT NULL,
                line_count INTEGER NOT NULL,
                section_count INTEGER NOT NULL,
                top_terms_json TEXT,
                sections_json TEXT,
                extraction_method TEXT,
                source_digest TEXT,
                sample_preview TEXT,
                FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS retrieval_decisions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                query TEXT NOT NULL,
                route TEXT NOT NULL,
                reason TEXT NOT NULL,
                signals_json TEXT,
                documents_considered_json TEXT,
                chosen_path TEXT,
                timestamp TEXT NOT NULL
            );

            CREATE TABLE IF NOT EXISTS audit_events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp TEXT NOT NULL,
                event_type TEXT NOT NULL,
                actor TEXT NOT NULL,
                payload_json TEXT
            );

            -- FTS5 Virtual Table for conditional deferred full-text search
            CREATE VIRTUAL TABLE IF NOT EXISTS document_text_fts USING fts5(
                document_id UNINDEXED,
                title,
                body_text
            );
        )");
        if (!m1_res) return m1_res;

        auto rec_res = db.prepare("INSERT INTO schema_migrations (version, name, applied_at) VALUES (1, 'initial_baseline_schema', ?);");
        if (!rec_res) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to record migration 1");
        rec_res->bindText(1, now_str);
        rec_res->step();
    }

    return core::makeOk();
}

} // namespace contextlab::persistence
