#pragma once

#include <memory>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "contextlab/core/Result.hpp"
#include "contextlab/domain/Document.hpp"
#include "contextlab/domain/DomainModels.hpp"
#include "contextlab/ingest/ContentAddressableStore.hpp"
#include "contextlab/ingest/Extractor.hpp"
#include "contextlab/metadata/SchemaRegistry.hpp"
#include "contextlab/persistence/Repository.hpp"

namespace contextlab::ingest {

struct IngestionReport {
    bool success{false};
    std::string document_id;
    std::string artifact_sha256;
    std::string source_file;
    std::string format_detected;
    bool has_declared_context{false};
    domain::ValidationState validation_state{domain::ValidationState::UNVALIDATED};
    std::string validation_error;
    std::string message;
    bool text_analysis_performed{false};
    nlohmann::json extracted_metadata = nlohmann::json::object();

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"success", success},
            {"document_id", document_id},
            {"artifact_sha256", artifact_sha256},
            {"source_file", source_file},
            {"format_detected", format_detected},
            {"has_declared_context", has_declared_context},
            {"validation_state", domain::validationStateToString(validation_state)},
            {"validation_error", validation_error},
            {"message", message},
            {"text_analysis_performed", text_analysis_performed},
            {"extracted_metadata", extracted_metadata}
        };
    }
};

class IngestionPipeline {
public:
    IngestionPipeline(ContentAddressableStore& cas, metadata::SchemaRegistry& schemas, persistence::Repository& repo);

    [[nodiscard]] core::Result<IngestionReport> ingestFile(const std::filesystem::path& file_path);
    [[nodiscard]] core::Result<IngestionReport> ingestContent(std::string_view content, const std::string& original_filename, const std::string& media_type = "");

private:
    [[nodiscard]] const IExtractor* findExtractor(const std::filesystem::path& path, const std::string& media_type) const;

    ContentAddressableStore& cas_;
    metadata::SchemaRegistry& schemas_;
    persistence::Repository& repo_;
    std::vector<std::unique_ptr<IExtractor>> extractors_;
};

} // namespace contextlab::ingest
