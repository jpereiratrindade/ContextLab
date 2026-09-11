#include "contextlab/core/Config.hpp"
#include <fstream>

namespace contextlab::core {

AppConfig AppConfig::loadFromFile(const std::filesystem::path& path) {
    AppConfig config;
    if (!std::filesystem::exists(path)) {
        return config;
    }

    try {
        std::ifstream file(path);
        nlohmann::json j;
        file >> j;

        if (j.contains("server")) {
            const auto& s = j["server"];
            if (s.contains("host") && s["host"].is_string()) config.server.host = s["host"].get<std::string>();
            if (s.contains("port") && s["port"].is_number()) config.server.port = s["port"].get<int>();
        }

        if (j.contains("storage")) {
            const auto& st = j["storage"];
            if (st.contains("root") && st["root"].is_string()) config.storage.root = st["root"].get<std::string>();
        }

        if (j.contains("analysis")) {
            const auto& a = j["analysis"];
            if (a.contains("auto_deepen") && a["auto_deepen"].is_boolean()) config.analysis.auto_deepen = a["auto_deepen"].get<bool>();
        }
    } catch (...) {
        // Fallback to default
    }

    return config;
}

void AppConfig::saveToFile(const std::filesystem::path& path) const {
    nlohmann::json j = {
        {"server", {
            {"host", server.host},
            {"port", server.port}
        }},
        {"storage", {
            {"root", storage.root.string()}
        }},
        {"analysis", {
            {"auto_deepen", analysis.auto_deepen}
        }}
    };

    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream file(path);
    file << j.dump(2) << "\n";
}

} // namespace contextlab::core
