#pragma once

#include <string>
#include <string_view>
#include <format>
#include <compare>

namespace contextlab::core {

template <typename Tag>
class StronglyTypedId {
public:
    StronglyTypedId() = default;
    explicit StronglyTypedId(std::string value) : value_(std::move(value)) {}

    [[nodiscard]] const std::string& value() const noexcept { return value_; }
    [[nodiscard]] std::string_view view() const noexcept { return value_; }
    [[nodiscard]] bool empty() const noexcept { return value_.empty(); }

    auto operator<=>(const StronglyTypedId&) const = default;
    bool operator==(const StronglyTypedId&) const = default;

private:
    std::string value_;
};

struct DocumentIdTag {};
struct DocumentVersionTag {};
struct ArtifactIdTag {};
struct ProjectIdTag {};
struct SchemaIdTag {};
struct RelationIdTag {};
struct IngestionIdTag {};
struct AnalysisIdTag {};

using DocumentId = StronglyTypedId<DocumentIdTag>;
using DocumentVersion = StronglyTypedId<DocumentVersionTag>;
using ArtifactId = StronglyTypedId<ArtifactIdTag>;
using ProjectId = StronglyTypedId<ProjectIdTag>;
using SchemaId = StronglyTypedId<SchemaIdTag>;
using RelationId = StronglyTypedId<RelationIdTag>;
using IngestionId = StronglyTypedId<IngestionIdTag>;
using AnalysisId = StronglyTypedId<AnalysisIdTag>;

} // namespace contextlab::core
