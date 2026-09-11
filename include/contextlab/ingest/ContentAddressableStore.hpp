#pragma once

#include <string>
#include <filesystem>
#include <span>
#include <cstdint>
#include "contextlab/core/Result.hpp"
#include "contextlab/domain/Artifact.hpp"

namespace contextlab::ingest {

class ContentAddressableStore {
public:
    explicit ContentAddressableStore(std::filesystem::path root_dir);

    [[nodiscard]] core::Result<domain::Artifact> storeFile(const std::filesystem::path& file_path);
    [[nodiscard]] core::Result<domain::Artifact> storeBytes(std::span<const uint8_t> data, const std::string& original_filename, const std::string& media_type);
    [[nodiscard]] core::Result<domain::Artifact> storeString(std::string_view data, const std::string& original_filename, const std::string& media_type);

    [[nodiscard]] bool hasObject(const std::string& sha256) const;
    [[nodiscard]] std::filesystem::path getObjectPath(const std::string& sha256) const;
    [[nodiscard]] core::Result<std::string> readObjectString(const std::string& sha256) const;
    [[nodiscard]] core::Result<std::vector<uint8_t>> readObjectBytes(const std::string& sha256) const;

    [[nodiscard]] const std::filesystem::path& rootDir() const noexcept { return root_dir_; }

private:
    [[nodiscard]] static std::string detectMediaType(const std::filesystem::path& path);

    std::filesystem::path root_dir_;
};

} // namespace contextlab::ingest
