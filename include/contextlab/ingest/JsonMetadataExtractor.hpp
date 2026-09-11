#pragma once

#include "contextlab/ingest/Extractor.hpp"

namespace contextlab::ingest {

class JsonMetadataExtractor : public IExtractor {
public:
    [[nodiscard]] bool canHandle(const std::filesystem::path& file_path, const std::string& media_type) const override;
    [[nodiscard]] core::Result<ExtractionResult> extract(const std::filesystem::path& file_path, const std::string& content) const override;
};

} // namespace contextlab::ingest
