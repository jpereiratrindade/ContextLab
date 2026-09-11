#pragma once

#include <string>
#include <string_view>
#include <span>
#include <cstdint>
#include <filesystem>

namespace contextlab::core {

class Sha256 {
public:
    Sha256();
    void update(std::span<const uint8_t> data);
    void update(std::string_view data);
    [[nodiscard]] std::string finalize();

    [[nodiscard]] static std::string hashString(std::string_view data);
    [[nodiscard]] static std::string hashBytes(std::span<const uint8_t> data);
    [[nodiscard]] static std::string hashFile(const std::filesystem::path& path);

private:
    void transform(const uint8_t chunk[64]);

    uint32_t state_[8];
    uint64_t bitlen_{0};
    uint8_t buffer_[64];
    size_t datalen_{0};
};

} // namespace contextlab::core
