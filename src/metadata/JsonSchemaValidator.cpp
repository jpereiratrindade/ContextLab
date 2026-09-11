#include "contextlab/metadata/JsonSchemaValidator.hpp"
#include <sstream>

namespace contextlab::metadata {

JsonSchemaValidator::JsonSchemaValidator(const nlohmann::json& schema_json) {
    (void)setSchema(schema_json);
}

core::Result<void> JsonSchemaValidator::setSchema(const nlohmann::json& schema_json) {
    try {
        schema_ = schema_json;
        validator_ = std::make_shared<nlohmann::json_schema::json_validator>();
        validator_->set_root_schema(schema_);
        return core::makeOk();
    } catch (const std::exception& e) {
        return core::makeError(core::ErrorCode::METADATA_SCHEMA_INVALID, std::string("Failed to compile JSON schema: ") + e.what());
    }
}

core::Result<void> JsonSchemaValidator::validate(const nlohmann::json& instance) const {
    if (!validator_) {
        return core::makeError(core::ErrorCode::METADATA_SCHEMA_UNKNOWN, "Validator has no root schema configured");
    }

    try {
        validator_->validate(instance);
        return core::makeOk();
    } catch (const std::exception& e) {
        return core::makeError(core::ErrorCode::METADATA_SCHEMA_INVALID, std::string("Validation failed: ") + e.what(), {{"schema_error", e.what()}});
    }
}

} // namespace contextlab::metadata
