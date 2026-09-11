#pragma once

#include <string>
#include <vector>
#include "contextlab/core/Result.hpp"
#include "contextlab/domain/DomainModels.hpp"
#include "contextlab/ingest/ContentAddressableStore.hpp"
#include "contextlab/persistence/Repository.hpp"

namespace contextlab::analysis {

class SelectiveDeepener {
public:
    SelectiveDeepener(ingest::ContentAddressableStore& cas, persistence::Repository& repo);

    [[nodiscard]] core::Result<domain::TextAnalysis> deepen(const std::string& document_id);

private:
    [[nodiscard]] static std::vector<std::string> extractSections(std::string_view text);
    [[nodiscard]] static std::vector<std::string> extractTopTerms(std::string_view text, size_t limit = 10);

    ingest::ContentAddressableStore& cas_;
    persistence::Repository& repo_;
};

} // namespace contextlab::analysis
