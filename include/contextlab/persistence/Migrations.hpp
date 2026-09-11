#pragma once

#include "contextlab/persistence/Database.hpp"
#include "contextlab/core/Result.hpp"

namespace contextlab::persistence {

class Migrations {
public:
    [[nodiscard]] static core::Result<void> applyAll(Database& db);
};

} // namespace contextlab::persistence
