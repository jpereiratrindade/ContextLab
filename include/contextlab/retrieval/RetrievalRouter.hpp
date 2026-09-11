#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "contextlab/domain/DomainModels.hpp"

namespace contextlab::retrieval {

class RetrievalRouter {
public:
    [[nodiscard]] static domain::RetrievalDecision classifyQuery(std::string_view query, const std::vector<std::string>& document_ids = {});
};

} // namespace contextlab::retrieval
