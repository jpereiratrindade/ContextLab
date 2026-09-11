#pragma once

#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "contextlab/core/Config.hpp"
#include "contextlab/core/Result.hpp"
#include "contextlab/domain/Document.hpp"
#include "contextlab/domain/DomainModels.hpp"
#include "contextlab/ingest/ContentAddressableStore.hpp"
#include "contextlab/ingest/IngestionPipeline.hpp"
#include "contextlab/metadata/SchemaRegistry.hpp"
#include "contextlab/persistence/Database.hpp"
#include "contextlab/persistence/Repository.hpp"
#include "contextlab/retrieval/SearchEngine.hpp"
#include "contextlab/analysis/SelectiveDeepener.hpp"

#include "contextlab/domain/User.hpp"

namespace contextlab::application {

struct VerificationGate {
    std::string name;
    bool passed{false};
    std::string message;
};

struct VerificationReport {
    bool all_passed{false};
    std::vector<VerificationGate> gates;
};

class ContextLabService {
public:
    ContextLabService(core::AppConfig config, std::filesystem::path repo_root);
    ~ContextLabService();

    [[nodiscard]] core::Result<void> initialize();
    [[nodiscard]] core::Result<void> initRepository();
    [[nodiscard]] VerificationReport runVerification();

    // User & Auth commands
    [[nodiscard]] core::Result<std::string> requestOtp(const std::string& email);
    [[nodiscard]] core::Result<std::pair<domain::User, std::string>> verifyOtp(const std::string& email, const std::string& otp_code);
    [[nodiscard]] core::Result<std::optional<domain::User>> authenticateToken(const std::string& token);
    [[nodiscard]] core::Result<void> logout(const std::string& token);
    [[nodiscard]] bool isDocumentAccessible(const domain::Document& doc, const std::optional<domain::User>& user) const;

    // Application commands (with user context / access control)
    [[nodiscard]] core::Result<ingest::IngestionReport> ingestFile(const std::filesystem::path& path);
    [[nodiscard]] core::Result<ingest::IngestionReport> ingestContent(std::string_view content, const std::string& filename, const std::string& media_type = "");
    [[nodiscard]] core::Result<domain::Document> getDocument(const std::string& id, const std::optional<domain::User>& user = std::nullopt);
    [[nodiscard]] core::Result<nlohmann::json> getDocumentFull(const std::string& id, const std::optional<domain::User>& user = std::nullopt);
    [[nodiscard]] core::Result<std::vector<domain::Document>> listDocuments(const std::optional<domain::User>& user = std::nullopt);
    [[nodiscard]] core::Result<void> updateDocument(const domain::Document& doc);
    [[nodiscard]] core::Result<void> deleteDocument(const std::string& id);

    [[nodiscard]] core::Result<std::vector<domain::Project>> listProjects();
    [[nodiscard]] core::Result<void> createProject(const domain::Project& project);
    [[nodiscard]] core::Result<void> updateProject(const domain::Project& project);
    [[nodiscard]] core::Result<void> deleteProject(const std::string& id);

    [[nodiscard]] core::Result<std::vector<domain::Relation>> listRelations();
    [[nodiscard]] core::Result<void> createRelation(const domain::Relation& relation);
    [[nodiscard]] core::Result<void> updateRelation(const domain::Relation& relation);
    [[nodiscard]] core::Result<void> deleteRelation(int64_t id);

    [[nodiscard]] core::Result<void> saveMetadataEnvelope(const domain::MetadataEnvelope& env);
    [[nodiscard]] core::Result<void> deleteMetadataEnvelope(const std::string& id);

    [[nodiscard]] core::Result<std::vector<domain::SchemaDefinition>> listSchemas();
    [[nodiscard]] core::Result<retrieval::SearchResponse> search(const std::string& query, const std::string& mode = "auto", const std::optional<domain::User>& user = std::nullopt);
    [[nodiscard]] core::Result<domain::TextAnalysis> deepen(const std::string& id);
    [[nodiscard]] core::Result<std::vector<domain::IngestionEvent>> getRecentEvents(int limit = 50);

    // Topic Graph & Analytics
    [[nodiscard]] core::Result<domain::TopicGraphData> getTopicGraph(const std::optional<domain::User>& user = std::nullopt);

    [[nodiscard]] nlohmann::json getHealthStatus();
    [[nodiscard]] nlohmann::json getSystemInfo();

    [[nodiscard]] const core::AppConfig& config() const noexcept { return config_; }
    [[nodiscard]] const std::filesystem::path& repoRoot() const noexcept { return repo_root_; }

private:
    core::AppConfig config_;
    std::filesystem::path repo_root_;
    std::unique_ptr<persistence::Database> db_;
    std::unique_ptr<persistence::Repository> repo_;
    std::unique_ptr<ingest::ContentAddressableStore> cas_;
    std::unique_ptr<metadata::SchemaRegistry> schemas_;
    std::unique_ptr<ingest::IngestionPipeline> pipeline_;
    std::unique_ptr<retrieval::SearchEngine> search_engine_;
    std::unique_ptr<analysis::SelectiveDeepener> deepener_;
};

} // namespace contextlab::application
