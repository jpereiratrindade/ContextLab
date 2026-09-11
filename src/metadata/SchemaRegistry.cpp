#include "contextlab/metadata/SchemaRegistry.hpp"
#include "contextlab/core/Sha256.hpp"
#include <fstream>
#include <chrono>
#include <format>

namespace contextlab::metadata {

std::string SchemaRegistry::makeKey(const std::string& id, const std::string& ver) const {
    if (ver.empty()) {
        return id;
    }
    return id + "@" + ver;
}

core::Result<void> SchemaRegistry::loadFromDirectory(const std::filesystem::path& dir_path) {
    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
        return core::makeOk();
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            try {
                std::ifstream f(entry.path());
                nlohmann::json schema_json;
                f >> schema_json;

                std::string schema_id;
                if (schema_json.contains("$id") && schema_json["$id"].is_string()) {
                    schema_id = schema_json["$id"].get<std::string>();
                } else if (schema_json.contains("id") && schema_json["id"].is_string()) {
                    schema_id = schema_json["id"].get<std::string>();
                } else {
                    schema_id = entry.path().stem().string();
                }

                std::string version = "0.1.0";
                if (schema_json.contains("version") && schema_json["version"].is_string()) {
                    version = schema_json["version"].get<std::string>();
                } else if (schema_id.find(':') != std::string::npos) {
                    // Extract version suffix if in urn format like urn:contextlab:document-context:0.1.0
                    auto last_colon = schema_id.rfind(':');
                    if (last_colon != std::string::npos && last_colon + 1 < schema_id.size()) {
                        version = schema_id.substr(last_colon + 1);
                    }
                }

                std::string raw_content = schema_json.dump();
                std::string digest = core::Sha256::hashString(raw_content);

                const auto now = std::chrono::system_clock::now();
                std::string reg_time = std::format("{:%Y-%m-%d %H:%M:%S}", now);

                domain::SchemaDefinition def{
                    .id = schema_id,
                    .version = version,
                    .digest = digest,
                    .source_path = entry.path().string(),
                    .schema_json = schema_json,
                    .active = true,
                    .registered_at = reg_time
                };

                auto res = registerSchema(std::move(def));
                if (!res) {
                    return res;
                }
            } catch (const std::exception& e) {
                return core::makeError(core::ErrorCode::METADATA_SCHEMA_INVALID,
                    std::string("Failed to load schema file ") + entry.path().string() + ": " + e.what());
            }
        }
    }

    return core::makeOk();
}

core::Result<void> SchemaRegistry::registerSchema(domain::SchemaDefinition schema) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto validator = std::make_shared<JsonSchemaValidator>();
    auto set_res = validator->setSchema(schema.schema_json);
    if (!set_res) {
        return set_res;
    }

    std::string full_key = makeKey(schema.id, schema.version);
    std::string id_key = schema.id;

    schemas_[full_key] = schema;
    schemas_[id_key] = schema;

    validators_[full_key] = validator;
    validators_[id_key] = validator;

    // Also register base urn without version suffix if present
    auto last_colon = schema.id.rfind(':');
    if (last_colon != std::string::npos && last_colon > 0) {
        std::string base_id = schema.id.substr(0, last_colon);
        schemas_[base_id] = schema;
        validators_[base_id] = validator;
    }

    return core::makeOk();
}

bool SchemaRegistry::hasSchema(const std::string& schema_id, const std::string& version) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!version.empty()) {
        std::string full_key = makeKey(schema_id, version);
        if (schemas_.find(full_key) != schemas_.end()) return true;
    }
    return schemas_.find(schema_id) != schemas_.end();
}

core::Result<domain::SchemaDefinition> SchemaRegistry::getSchema(const std::string& schema_id, const std::string& version) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!version.empty()) {
        std::string full_key = makeKey(schema_id, version);
        auto it = schemas_.find(full_key);
        if (it != schemas_.end()) return core::makeOk(it->second);
    }
    auto it = schemas_.find(schema_id);
    if (it != schemas_.end()) return core::makeOk(it->second);

    return core::makeError(core::ErrorCode::METADATA_SCHEMA_UNKNOWN, "Schema not found: " + schema_id);
}

std::vector<domain::SchemaDefinition> SchemaRegistry::listSchemas() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<domain::SchemaDefinition> result;
    std::unordered_map<std::string, bool> seen;
    for (const auto& [k, v] : schemas_) {
        if (!seen[v.id + "@" + v.version]) {
            seen[v.id + "@" + v.version] = true;
            result.push_back(v);
        }
    }
    return result;
}

core::Result<void> SchemaRegistry::validatePayload(const std::string& schema_id, const std::string& version, const nlohmann::json& payload) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<JsonSchemaValidator> validator;

    if (!version.empty()) {
        auto it = validators_.find(makeKey(schema_id, version));
        if (it != validators_.end()) {
            validator = it->second;
        }
    }
    if (!validator) {
        auto it = validators_.find(schema_id);
        if (it != validators_.end()) {
            validator = it->second;
        }
    }

    if (!validator) {
        // Also check if schema_id is urn prefix
        for (const auto& [k, v] : validators_) {
            if (k.starts_with(schema_id)) {
                validator = v;
                break;
            }
        }
    }

    if (!validator) {
        return core::makeError(core::ErrorCode::METADATA_SCHEMA_UNKNOWN, "No validator found for schema ID: " + schema_id);
    }

    return validator->validate(payload);
}

} // namespace contextlab::metadata
