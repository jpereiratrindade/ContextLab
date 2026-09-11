#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "contextlab/core/Types.hpp"
#include "contextlab/domain/Authority.hpp"
#include "contextlab/domain/MetadataEnvelope.hpp"
#include "contextlab/domain/Artifact.hpp"

namespace contextlab::domain {

struct Document {
    std::string id{};
    std::string title{};
    std::string subtitle{};
    std::string version{};
    std::string date_created{};
    std::string date_modified{};
    std::string language{};
    std::string document_type{};
    std::string lifecycle_state{};
    std::string publication_state{};
    std::string primary_project{};
    std::string resource_scope{};
    bool self_describing{true};
    bool self_consumption_required{false};
    std::string epistemic_status{};
    Authority primary_authority{Authority::DECLARED};
    bool text_analyzed{false};
    std::string created_at{};
    std::string owner{""};
    std::string visibility{"public"}; // public, internal_embrapa, team, private
    std::vector<std::string> allowed_teams{};
    std::vector<std::string> allowed_users{};

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"title", title},
            {"subtitle", subtitle},
            {"version", version},
            {"date_created", date_created},
            {"date_modified", date_modified},
            {"language", language},
            {"document_type", document_type},
            {"lifecycle_state", lifecycle_state},
            {"publication_state", publication_state},
            {"primary_project", primary_project},
            {"resource_scope", resource_scope},
            {"self_describing", self_describing},
            {"self_consumption_required", self_consumption_required},
            {"epistemic_status", epistemic_status},
            {"primary_authority", authorityToString(primary_authority)},
            {"text_analyzed", text_analyzed},
            {"created_at", created_at},
            {"owner", owner},
            {"visibility", visibility},
            {"allowed_teams", allowed_teams},
            {"allowed_users", allowed_users}
        };
    }
};

} // namespace contextlab::domain
