#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "contextlab/core/Types.hpp"
#include "contextlab/domain/Authority.hpp"

namespace contextlab::domain {

struct MetadataEnvelope {
    std::string id;
    std::string document_id;
    Authority authority{Authority::DECLARED};
    std::string schema_id;
    std::string schema_version;
    nlohmann::json payload;
    std::string producer;
    std::string method;
    double confidence{1.0};
    std::string source_artifact_sha256;
    std::string created_at;
    ValidationState validation_state{ValidationState::UNVALIDATED};
    std::string validation_error;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"document_id", document_id},
            {"authority", authorityToString(authority)},
            {"schema_id", schema_id},
            {"schema_version", schema_version},
            {"payload", payload},
            {"producer", producer},
            {"method", method},
            {"confidence", confidence},
            {"source_artifact_sha256", source_artifact_sha256},
            {"created_at", created_at},
            {"validation_state", validationStateToString(validation_state)},
            {"validation_error", validation_error}
        };
    }
};

} // namespace contextlab::domain
