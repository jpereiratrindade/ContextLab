#pragma once

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace contextlab::core {

struct ServerConfig {
    std::string host{"127.0.0.1"};
    int port{8080};
};

struct StorageConfig {
    std::filesystem::path root{"data"};

    [[nodiscard]] std::filesystem::path databasePath() const {
        return root / "contextlab.db";
    }

    [[nodiscard]] std::filesystem::path objectsPath() const {
        return root / "objects" / "sha256";
    }
};

struct AnalysisConfig {
    bool auto_deepen{false};
};

struct AppConfig {
    ServerConfig server;
    StorageConfig storage;
    AnalysisConfig analysis;

    [[nodiscard]] static AppConfig loadFromFile(const std::filesystem::path& path);
    void saveToFile(const std::filesystem::path& path) const;
};

} // namespace contextlab::core
