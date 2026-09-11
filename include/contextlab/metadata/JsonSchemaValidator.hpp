#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include "contextlab/core/Result.hpp"

namespace contextlab::metadata {

class JsonSchemaValidator {
public:
    JsonSchemaValidator() = default;
    explicit JsonSchemaValidator(const nlohmann::json& schema_json);

    [[nodiscard]] core::Result<void> setSchema(const nlohmann::json& schema_json);
    [[nodiscard]] core::Result<void> validate(const nlohmann::json& instance) const;

private:
    std::shared_ptr<nlohmann::json_schema::json_validator> validator_;
    nlohmann::json schema_;
};

} // namespace contextlab::metadata
