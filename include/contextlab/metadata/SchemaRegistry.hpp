#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <mutex>
#include "contextlab/core/Result.hpp"
#include "contextlab/domain/DomainModels.hpp"
#include "contextlab/metadata/JsonSchemaValidator.hpp"

namespace contextlab::metadata {

class SchemaRegistry {
public:
    SchemaRegistry() = default;

    [[nodiscard]] core::Result<void> loadFromDirectory(const std::filesystem::path& dir_path);
    [[nodiscard]] core::Result<void> registerSchema(domain::SchemaDefinition schema);

    [[nodiscard]] bool hasSchema(const std::string& schema_id, const std::string& version = "") const;
    [[nodiscard]] core::Result<domain::SchemaDefinition> getSchema(const std::string& schema_id, const std::string& version = "") const;
    [[nodiscard]] std::vector<domain::SchemaDefinition> listSchemas() const;

    [[nodiscard]] core::Result<void> validatePayload(const std::string& schema_id, const std::string& version, const nlohmann::json& payload) const;

private:
    [[nodiscard]] std::string makeKey(const std::string& id, const std::string& ver) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, domain::SchemaDefinition> schemas_;
    std::unordered_map<std::string, std::shared_ptr<JsonSchemaValidator>> validators_;
};

} // namespace contextlab::metadata
