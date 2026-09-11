#include "contextlab/ingest/ContentAddressableStore.hpp"
#include "contextlab/core/Sha256.hpp"
#include <fstream>
#include <chrono>
#include <format>

namespace contextlab::ingest {

ContentAddressableStore::ContentAddressableStore(std::filesystem::path root_dir)
    : root_dir_(std::move(root_dir)) {
    std::filesystem::create_directories(root_dir_);
}

std::filesystem::path ContentAddressableStore::getObjectPath(const std::string& sha256) const {
    if (sha256.size() < 4) {
        return root_dir_ / sha256;
    }
    std::string prefix = sha256.substr(0, 2);
    return root_dir_ / prefix / sha256;
}

bool ContentAddressableStore::hasObject(const std::string& sha256) const {
    return std::filesystem::exists(getObjectPath(sha256));
}

std::string ContentAddressableStore::detectMediaType(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (ext == ".md" || ext == ".markdown") return "text/markdown";
    if (ext == ".json") return "application/json";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".tex" || ext == ".latex") return "application/x-tex";
    if (ext == ".txt") return "text/plain";
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".yaml" || ext == ".yml") return "application/yaml";
    return "application/octet-stream";
}

core::Result<domain::Artifact> ContentAddressableStore::storeFile(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) {
        return core::makeError(core::ErrorCode::FILE_NOT_FOUND, "File does not exist: " + file_path.string());
    }

    std::string sha = core::Sha256::hashFile(file_path);
    if (sha.empty()) {
        return core::makeError(core::ErrorCode::FILE_NOT_FOUND, "Unable to read file for hashing: " + file_path.string());
    }

    auto dest_path = getObjectPath(sha);
    if (!std::filesystem::exists(dest_path)) {
        std::filesystem::create_directories(dest_path.parent_path());
        std::filesystem::copy_file(file_path, dest_path, std::filesystem::copy_options::overwrite_existing);
    }

    const auto now = std::chrono::system_clock::now();
    std::string time_str = std::format("{:%Y-%m-%d %H:%M:%S}", now);

    return core::makeOk(domain::Artifact{
        .sha256 = sha,
        .original_filename = file_path.filename().string(),
        .media_type = detectMediaType(file_path),
        .size_bytes = std::filesystem::file_size(file_path),
        .ingested_at = time_str,
        .object_path = dest_path.string()
    });
}

core::Result<domain::Artifact> ContentAddressableStore::storeBytes(std::span<const uint8_t> data, const std::string& original_filename, const std::string& media_type) {
    std::string sha = core::Sha256::hashBytes(data);
    auto dest_path = getObjectPath(sha);

    if (!std::filesystem::exists(dest_path)) {
        std::filesystem::create_directories(dest_path.parent_path());
        std::ofstream out(dest_path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }

    const auto now = std::chrono::system_clock::now();
    std::string time_str = std::format("{:%Y-%m-%d %H:%M:%S}", now);

    return core::makeOk(domain::Artifact{
        .sha256 = sha,
        .original_filename = original_filename,
        .media_type = media_type.empty() ? detectMediaType(original_filename) : media_type,
        .size_bytes = data.size(),
        .ingested_at = time_str,
        .object_path = dest_path.string()
    });
}

core::Result<domain::Artifact> ContentAddressableStore::storeString(std::string_view data, const std::string& original_filename, const std::string& media_type) {
    return storeBytes(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()), original_filename, media_type);
}

core::Result<std::string> ContentAddressableStore::readObjectString(const std::string& sha256) const {
    auto path = getObjectPath(sha256);
    if (!std::filesystem::exists(path)) {
        return core::makeError(core::ErrorCode::FILE_NOT_FOUND, "CAS object not found: " + sha256);
    }
    std::ifstream in(path, std::ios::binary);
    std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return core::makeOk(str);
}

core::Result<std::vector<uint8_t>> ContentAddressableStore::readObjectBytes(const std::string& sha256) const {
    auto path = getObjectPath(sha256);
    if (!std::filesystem::exists(path)) {
        return core::makeError(core::ErrorCode::FILE_NOT_FOUND, "CAS object not found: " + sha256);
    }
    std::ifstream in(path, std::ios::binary);
    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    in.read(reinterpret_cast<char*>(buffer.data()), size);
    return core::makeOk(buffer);
}

} // namespace contextlab::ingest
