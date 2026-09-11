#pragma once

#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "contextlab/core/Types.hpp"

namespace contextlab::domain {

struct Artifact {
    std::string sha256;
    std::string original_filename;
    std::string media_type;
    uint64_t size_bytes{0};
    std::string ingested_at;
    std::string object_path;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"sha256", sha256},
            {"original_filename", original_filename},
            {"media_type", media_type},
            {"size_bytes", size_bytes},
            {"ingested_at", ingested_at},
            {"object_path", object_path}
        };
    }
};

} // namespace contextlab::domain
