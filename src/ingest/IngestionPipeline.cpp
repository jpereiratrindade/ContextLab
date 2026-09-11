#include "contextlab/ingest/IngestionPipeline.hpp"
#include "contextlab/ingest/MarkdownExtractor.hpp"
#include "contextlab/ingest/JsonMetadataExtractor.hpp"
#include "contextlab/ingest/LatexExtractor.hpp"
#include "contextlab/ingest/TxtExtractor.hpp"
#include "contextlab/ingest/PdfExtractor.hpp"
#include "contextlab/core/Logger.hpp"
#include <fstream>
#include <chrono>
#include <format>

namespace contextlab::ingest {

IngestionPipeline::IngestionPipeline(ContentAddressableStore& cas, metadata::SchemaRegistry& schemas, persistence::Repository& repo)
    : cas_(cas), schemas_(schemas), repo_(repo) {
    extractors_.push_back(std::make_unique<MarkdownExtractor>());
    extractors_.push_back(std::make_unique<JsonMetadataExtractor>());
    extractors_.push_back(std::make_unique<LatexExtractor>());
    extractors_.push_back(std::make_unique<PdfExtractor>());
    extractors_.push_back(std::make_unique<TxtExtractor>());
}

const IExtractor* IngestionPipeline::findExtractor(const std::filesystem::path& path, const std::string& media_type) const {
    for (const auto& ext : extractors_) {
        if (ext->canHandle(path, media_type)) {
            return ext.get();
        }
    }
    return nullptr;
}

core::Result<IngestionReport> IngestionPipeline::ingestFile(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) {
        return core::makeError(core::ErrorCode::FILE_NOT_FOUND, "File not found: " + file_path.string());
    }

    auto store_res = cas_.storeFile(file_path);
    if (!store_res) return core::makeError(store_res.error().code, store_res.error().message);
    const auto& artifact = *store_res;

    return ingestArtifact(artifact, file_path);
}

core::Result<IngestionReport> IngestionPipeline::ingestContent(std::string_view content, const std::string& original_filename, const std::string& media_type) {
    auto store_res = cas_.storeString(content, original_filename, media_type);
    if (!store_res) return core::makeError(store_res.error().code, store_res.error().message);
    const auto& artifact = *store_res;

    std::string fname = original_filename.empty() ? "upload.bin" : original_filename;
    return ingestArtifact(artifact, std::filesystem::path(fname));
}

core::Result<IngestionReport> IngestionPipeline::ingestArtifact(const domain::Artifact& artifact, const std::filesystem::path& original_source_path) {
    // Save artifact in DB
    auto art_save_res = repo_.saveArtifact(artifact);
    if (!art_save_res) return std::unexpected(art_save_res.error());

    auto cas_file_path = cas_.getObjectPath(artifact.sha256);

    std::string content;
    if (artifact.media_type != "application/pdf") {
        std::ifstream in(cas_file_path, std::ios::binary);
        content.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }

    const auto* extractor = findExtractor(original_source_path, artifact.media_type);
    if (!extractor) {
        extractor = findExtractor(cas_file_path, artifact.media_type);
    }
    if (!extractor) {
        return core::makeError(core::ErrorCode::FORMAT_UNSUPPORTED, "No suitable extractor for: " + original_source_path.string());
    }

    auto ext_res = extractor->extract(cas_file_path, content);
    if (!ext_res) {
        // Record failure event
        const auto now = std::chrono::system_clock::now();
        std::string now_str = std::format("{:%Y-%m-%d %H:%M:%S}", now);

        domain::IngestionEvent ev{
            .timestamp = now_str,
            .source_file = original_source_path.filename().string(),
            .artifact_sha256 = artifact.sha256,
            .document_id = "",
            .status = "FAILED",
            .message = ext_res.error().message,
            .details = ext_res.error().details
        };
        (void)repo_.recordIngestionEvent(ev);

        return std::unexpected(ext_res.error());
    }

    const auto& extracted = *ext_res;
    const auto now = std::chrono::system_clock::now();
    std::string now_str = std::format("{:%Y-%m-%d %H:%M:%S}", now);

    IngestionReport report;
    report.success = true;
    report.artifact_sha256 = artifact.sha256;
    report.source_file = original_source_path.filename().string();
    report.format_detected = extracted.format_detected;
    report.has_declared_context = extracted.has_declared_context;
    report.text_analysis_performed = false;
    report.extracted_metadata = extracted.metadata_payload;

    if (extracted.has_declared_context) {
        const auto& payload = extracted.metadata_payload;

        std::string schema_id = "urn:contextlab:document-context";
        std::string schema_ver = "0.1.0";
        if (payload.contains("schema") && payload["schema"].is_object()) {
            if (payload["schema"].contains("id") && payload["schema"]["id"].is_string()) {
                schema_id = payload["schema"]["id"].get<std::string>();
            }
            if (payload["schema"].contains("version") && payload["schema"]["version"].is_string()) {
                schema_ver = payload["schema"]["version"].get<std::string>();
            }
        }

        // Validate against registered schema
        auto val_res = schemas_.validatePayload(schema_id, schema_ver, payload);
        domain::ValidationState val_state = domain::ValidationState::VALID;
        std::string val_err;

        if (!val_res) {
            val_state = (val_res.error().code == core::ErrorCode::METADATA_SCHEMA_UNKNOWN)
                ? domain::ValidationState::UNKNOWN_SCHEMA
                : domain::ValidationState::INVALID;
            val_err = val_res.error().message;
        }

        report.validation_state = val_state;
        report.validation_error = val_err;

        // Resolve document identity from payload
        std::string doc_id = original_source_path.stem().string();
        std::string title = original_source_path.filename().string();
        std::string subtitle = "";
        std::string version = "0.1.0";
        std::string date_created = now_str;
        std::string date_modified = now_str;
        std::string language = "pt-BR";
        std::string doc_type = "research_document";
        std::string lifecycle_state = "active";
        std::string publication_state = "internal";
        std::string primary_project = "ContextLab";
        std::string resource_scope = "document";
        bool self_describing = true;
        bool self_consumption_required = false;
        std::string epistemic_status = "unspecified";

        if (payload.contains("document") && payload["document"].is_object()) {
            const auto& doc_meta = payload["document"];
            if (doc_meta.contains("id") && doc_meta["id"].is_string()) doc_id = doc_meta["id"].get<std::string>();
            if (doc_meta.contains("title") && doc_meta["title"].is_string()) title = doc_meta["title"].get<std::string>();
            if (doc_meta.contains("subtitle") && doc_meta["subtitle"].is_string()) subtitle = doc_meta["subtitle"].get<std::string>();
            if (doc_meta.contains("version") && doc_meta["version"].is_string()) version = doc_meta["version"].get<std::string>();
            if (doc_meta.contains("date_created") && doc_meta["date_created"].is_string()) date_created = doc_meta["date_created"].get<std::string>();
            if (doc_meta.contains("date_modified") && doc_meta["date_modified"].is_string()) date_modified = doc_meta["date_modified"].get<std::string>();
            if (doc_meta.contains("language") && doc_meta["language"].is_string()) language = doc_meta["language"].get<std::string>();
            if (doc_meta.contains("document_type") && doc_meta["document_type"].is_string()) doc_type = doc_meta["document_type"].get<std::string>();
            if (doc_meta.contains("lifecycle_state") && doc_meta["lifecycle_state"].is_string()) lifecycle_state = doc_meta["lifecycle_state"].get<std::string>();
            if (doc_meta.contains("publication_state") && doc_meta["publication_state"].is_string()) publication_state = doc_meta["publication_state"].get<std::string>();
            if (doc_meta.contains("primary_project") && doc_meta["primary_project"].is_string()) primary_project = doc_meta["primary_project"].get<std::string>();
            if (doc_meta.contains("resource_scope") && doc_meta["resource_scope"].is_string()) resource_scope = doc_meta["resource_scope"].get<std::string>();
            if (doc_meta.contains("self_describing") && doc_meta["self_describing"].is_boolean()) self_describing = doc_meta["self_describing"].get<bool>();
            if (doc_meta.contains("self_consumption_required") && doc_meta["self_consumption_required"].is_boolean()) self_consumption_required = doc_meta["self_consumption_required"].get<bool>();
        }

        if (payload.contains("epistemic_context") && payload["epistemic_context"].is_object()) {
            if (payload["epistemic_context"].contains("status") && payload["epistemic_context"]["status"].is_string()) {
                epistemic_status = payload["epistemic_context"]["status"].get<std::string>();
            }
        }

        report.document_id = doc_id;

        domain::Document doc{
            .id = doc_id,
            .title = title,
            .subtitle = subtitle,
            .version = version,
            .date_created = date_created,
            .date_modified = date_modified,
            .language = language,
            .document_type = doc_type,
            .lifecycle_state = lifecycle_state,
            .publication_state = publication_state,
            .primary_project = primary_project,
            .resource_scope = resource_scope,
            .self_describing = self_describing,
            .self_consumption_required = self_consumption_required,
            .epistemic_status = epistemic_status,
            .primary_authority = domain::Authority::DECLARED,
            .text_analyzed = false,
            .created_at = now_str
        };

        auto doc_save_res = repo_.saveDocument(doc);
        if (!doc_save_res) return std::unexpected(doc_save_res.error());

        (void)repo_.linkDocumentArtifact(doc_id, artifact.sha256, "primary_source");

        // Save metadata envelope
        domain::MetadataEnvelope env{
            .id = doc_id + "-env-0",
            .document_id = doc_id,
            .authority = domain::Authority::DECLARED,
            .schema_id = schema_id,
            .schema_version = schema_ver,
            .payload = payload,
            .producer = "author_declared",
            .method = "document_header_extraction",
            .confidence = 1.0,
            .source_artifact_sha256 = artifact.sha256,
            .created_at = now_str,
            .validation_state = val_state,
            .validation_error = val_err
        };
        (void)repo_.saveMetadataEnvelope(env);

        // Process project context
        if (payload.contains("project_context") && payload["project_context"].is_object()) {
            const auto& p = payload["project_context"];
            std::string p_name = primary_project;
            if (p.contains("project") && p["project"].is_string()) p_name = p["project"].get<std::string>();
            std::string p_kind = p.value("project_kind", "research_project");
            std::string p_stage = p.value("research_stage", "active");
            std::string p_study = p.value("object_of_study", "");
            std::string p_question = p.value("central_question", "");
            std::string p_eng_question = p.value("engineering_question", "");

            std::vector<std::string> p_domains;
            if (p.contains("research_domain") && p["research_domain"].is_array()) {
                p_domains = p["research_domain"].get<std::vector<std::string>>();
            }

            domain::Project project{
                .id = primary_project,
                .name = p_name,
                .kind = p_kind,
                .research_domain = std::move(p_domains),
                .stage = p_stage,
                .object_of_study = p_study,
                .central_question = p_question,
                .engineering_question = p_eng_question
            };
            (void)repo_.saveProject(project);
            (void)repo_.linkDocumentProject(doc_id, project.id);
        }

        // Clean previous relations and concepts for idempotent re-ingest
        (void)repo_.db().execute("DELETE FROM relations WHERE document_id = '" + doc_id + "';");
        (void)repo_.db().execute("DELETE FROM document_concepts WHERE document_id = '" + doc_id + "';");

        // Process semantic concepts
        if (payload.contains("semantic_context") && payload["semantic_context"].is_object()) {
            const auto& sc = payload["semantic_context"];
            if (sc.contains("primary_concepts") && sc["primary_concepts"].is_array()) {
                for (const auto& c_val : sc["primary_concepts"]) {
                    if (c_val.is_object() && c_val.contains("id") && c_val.contains("label")) {
                        domain::Concept concept_obj{
                            .id = c_val["id"].get<std::string>(),
                            .label = c_val["label"].get<std::string>(),
                            .role = c_val.value("role", "concept")
                        };
                        (void)repo_.saveConcept(concept_obj);
                        (void)repo_.linkDocumentConcept(doc_id, concept_obj.id);
                    }
                }
            }
        }

        // Process relations
        if (payload.contains("relations") && payload["relations"].is_array()) {
            for (const auto& r_val : payload["relations"]) {
                if (r_val.is_object() && r_val.contains("subject") && r_val.contains("predicate") && r_val.contains("object")) {
                    domain::Relation relation{
                        .subject = r_val["subject"].get<std::string>(),
                        .predicate = r_val["predicate"].get<std::string>(),
                        .object = r_val["object"].get<std::string>(),
                        .document_id = doc_id
                    };
                    (void)repo_.saveRelation(relation);
                }
            }
        }

        report.message = "Ingested self-describing document successfully with declared metadata.";
    } else {
        // Document without declared metadata
        std::string doc_id = original_source_path.stem().string();
        if (doc_id.empty() || doc_id == artifact.sha256) {
            doc_id = "DOC-" + artifact.sha256.substr(0, 8);
        }
        report.document_id = doc_id;

        std::string title = original_source_path.filename().string();
        if (title.empty()) title = artifact.original_filename.empty() ? ("Document " + artifact.sha256.substr(0, 8)) : artifact.original_filename;

        domain::Document doc{
            .id = doc_id,
            .title = std::move(title),
            .subtitle = "",
            .version = "0.1.0",
            .date_created = now_str,
            .date_modified = now_str,
            .language = "unknown",
            .document_type = (extracted.format_detected == "pdf" ? "pdf_artifact" : "unstructured_artifact"),
            .lifecycle_state = "active",
            .publication_state = "internal",
            .primary_project = "General",
            .resource_scope = "file",
            .self_describing = false,
            .self_consumption_required = false,
            .epistemic_status = "unstructured_source",
            .primary_authority = domain::Authority::EXTERNAL,
            .text_analyzed = false,
            .created_at = now_str
        };

        (void)repo_.saveDocument(doc);
        (void)repo_.linkDocumentArtifact(doc_id, artifact.sha256, "primary_source");

        report.validation_state = domain::ValidationState::UNVALIDATED;
        report.message = "Ingested artifact without declared metadata (" + extracted.notes + ")";
    }

    // Record ingestion event
    domain::IngestionEvent ev{
        .timestamp = now_str,
        .source_file = original_source_path.filename().string(),
        .artifact_sha256 = artifact.sha256,
        .document_id = report.document_id,
        .status = "SUCCESS",
        .message = report.message,
        .details = report.toJson()
    };
    (void)repo_.recordIngestionEvent(ev);

    core::Logger::instance().info("DOCUMENT_INGESTED", report.message, report.toJson());

    return core::makeOk(std::move(report));
}

} // namespace contextlab::ingest
