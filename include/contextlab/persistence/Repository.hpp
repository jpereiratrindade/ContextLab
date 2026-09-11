#pragma once

#include <string>
#include <vector>
#include <optional>
#include "contextlab/core/Result.hpp"
#include "contextlab/persistence/Database.hpp"
#include "contextlab/domain/Document.hpp"
#include "contextlab/domain/DomainModels.hpp"
#include "contextlab/domain/User.hpp"

namespace contextlab::persistence {

class Repository {
public:
    explicit Repository(Database& db) : db_(db) {}

    // User & Auth operations
    [[nodiscard]] core::Result<void> saveUser(const domain::User& user);
    [[nodiscard]] core::Result<std::optional<domain::User>> getUser(const std::string& email);
    [[nodiscard]] core::Result<std::vector<domain::User>> getAllUsers();
    [[nodiscard]] core::Result<void> saveOtp(const std::string& email, const std::string& otp_code, int expire_minutes = 10);
    [[nodiscard]] core::Result<bool> verifyOtp(const std::string& email, const std::string& otp_code);
    [[nodiscard]] core::Result<void> createSession(const std::string& email, const std::string& token, int expire_hours = 24);
    [[nodiscard]] core::Result<std::optional<domain::User>> getSessionUser(const std::string& token);
    [[nodiscard]] core::Result<void> deleteSession(const std::string& token);

    // Document operations
    [[nodiscard]] core::Result<void> saveDocument(const domain::Document& doc);
    [[nodiscard]] core::Result<void> updateDocument(const domain::Document& doc);
    [[nodiscard]] core::Result<void> deleteDocument(const std::string& doc_id);
    [[nodiscard]] core::Result<std::optional<domain::Document>> getDocument(const std::string& doc_id);
    [[nodiscard]] core::Result<std::vector<domain::Document>> getAllDocuments();
    [[nodiscard]] core::Result<size_t> countDocuments();
    [[nodiscard]] core::Result<void> updateDocumentTextAnalyzed(const std::string& doc_id, bool analyzed);

    // Artifact operations
    [[nodiscard]] core::Result<void> saveArtifact(const domain::Artifact& artifact);
    [[nodiscard]] core::Result<std::optional<domain::Artifact>> getArtifact(const std::string& sha256);
    [[nodiscard]] core::Result<std::vector<domain::Artifact>> getAllArtifacts();
    [[nodiscard]] core::Result<void> linkDocumentArtifact(const std::string& doc_id, const std::string& sha256, const std::string& role);
    [[nodiscard]] core::Result<std::vector<domain::Artifact>> getArtifactsForDocument(const std::string& doc_id);

    // Metadata Envelope operations
    [[nodiscard]] core::Result<void> saveMetadataEnvelope(const domain::MetadataEnvelope& env);
    [[nodiscard]] core::Result<void> deleteMetadataEnvelope(const std::string& id);
    [[nodiscard]] core::Result<std::vector<domain::MetadataEnvelope>> getMetadataEnvelopes(const std::string& doc_id);
    [[nodiscard]] core::Result<std::vector<domain::MetadataEnvelope>> getAllMetadataEnvelopes();

    // Project operations
    [[nodiscard]] core::Result<void> saveProject(const domain::Project& project);
    [[nodiscard]] core::Result<void> deleteProject(const std::string& project_id);
    [[nodiscard]] core::Result<std::optional<domain::Project>> getProject(const std::string& project_id);
    [[nodiscard]] core::Result<std::vector<domain::Project>> getAllProjects();
    [[nodiscard]] core::Result<void> linkDocumentProject(const std::string& doc_id, const std::string& project_id);
    [[nodiscard]] core::Result<void> unlinkDocumentProject(const std::string& doc_id, const std::string& project_id);

    // Concept operations
    [[nodiscard]] core::Result<void> saveConcept(const domain::Concept& concept_item);
    [[nodiscard]] core::Result<void> deleteConcept(const std::string& concept_id);
    [[nodiscard]] core::Result<std::vector<domain::Concept>> getConceptsForDocument(const std::string& doc_id);
    [[nodiscard]] core::Result<void> linkDocumentConcept(const std::string& doc_id, const std::string& concept_id);
    [[nodiscard]] core::Result<void> unlinkDocumentConcept(const std::string& doc_id, const std::string& concept_id);

    // Relation operations
    [[nodiscard]] core::Result<void> saveRelation(const domain::Relation& relation);
    [[nodiscard]] core::Result<void> updateRelation(const domain::Relation& relation);
    [[nodiscard]] core::Result<void> deleteRelation(int64_t relation_id);
    [[nodiscard]] core::Result<std::vector<domain::Relation>> getRelationsForDocument(const std::string& doc_id);
    [[nodiscard]] core::Result<std::vector<domain::Relation>> getAllRelations();

    // Event operations
    [[nodiscard]] core::Result<int64_t> recordIngestionEvent(const domain::IngestionEvent& ev);
    [[nodiscard]] core::Result<std::vector<domain::IngestionEvent>> getRecentIngestionEvents(int limit = 50);

    // Retrieval decisions
    [[nodiscard]] core::Result<int64_t> recordRetrievalDecision(const domain::RetrievalDecision& dec);
    [[nodiscard]] core::Result<std::vector<domain::RetrievalDecision>> getRecentRetrievalDecisions(int limit = 50);

    // Text analysis operations
    [[nodiscard]] core::Result<void> saveTextAnalysis(const domain::TextAnalysis& analysis);
    [[nodiscard]] core::Result<std::optional<domain::TextAnalysis>> getTextAnalysis(const std::string& doc_id);

    // FTS5 Full Text Indexing
    [[nodiscard]] core::Result<void> indexDocumentText(const std::string& doc_id, const std::string& title, const std::string& body_text);
    [[nodiscard]] core::Result<std::vector<std::pair<std::string, std::string>>> searchFts(const std::string& term);

    [[nodiscard]] Database& db() noexcept { return db_; }

private:
    Database& db_;
};

} // namespace contextlab::persistence
