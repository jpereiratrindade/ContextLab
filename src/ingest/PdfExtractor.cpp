#include "contextlab/ingest/PdfExtractor.hpp"
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFFileSpecObjectHelper.hh>
#include <qpdf/QPDFEmbeddedFileDocumentHelper.hh>
#include <qpdf/Buffer.hh>
#include <fstream>
#include <algorithm>

namespace contextlab::ingest {

bool PdfExtractor::canHandle(const std::filesystem::path& file_path, const std::string& media_type) const {
    if (media_type == "application/pdf") return true;
    std::string ext = file_path.extension().string();
    return ext == ".pdf";
}

core::Result<ExtractionResult> PdfExtractor::extract(const std::filesystem::path& file_path, const std::string& content) const {
    ExtractionResult result;
    result.format_detected = "pdf";

    try {
        QPDF qpdf;
        if (!content.empty() && !std::filesystem::exists(file_path)) {
            qpdf.processMemoryFile(file_path.string().c_str(), content.c_str(), content.size());
        } else {
            qpdf.processFile(file_path.string().c_str());
        }

        // Search for embedded attachments using QPDFEmbeddedFileDocumentHelper
        QPDFEmbeddedFileDocumentHelper efdh(qpdf);
        std::map<std::string, std::shared_ptr<QPDFFileSpecObjectHelper>> attachments;
        if (efdh.hasEmbeddedFiles()) {
            attachments = efdh.getEmbeddedFiles();
        }

        std::vector<std::pair<std::string, std::string>> matching_attachments;

        for (auto& [name, file_spec] : attachments) {
            if (!file_spec) continue;
            std::string filename = file_spec->getFilename();
            if (filename.empty()) filename = name;

            std::string lower_fname = filename;
            std::transform(lower_fname.begin(), lower_fname.end(), lower_fname.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

            if (lower_fname.ends_with(".metadata.json") ||
                lower_fname == "metadata.json" ||
                lower_fname == "document-context.json") {

                auto stream_obj = file_spec->getEmbeddedFileStream();
                if (stream_obj.isStream()) {
                    auto buf = stream_obj.getStreamData();
                    if (buf && buf->getSize() > 0) {
                        std::string payload_str(reinterpret_cast<const char*>(buf->getBuffer()), buf->getSize());
                        matching_attachments.emplace_back(filename, std::move(payload_str));
                    }
                }
            }
        }

        if (matching_attachments.empty()) {
            result.has_declared_context = false;
            result.notes = "PDF ingested successfully; declared context is absent_or_incomplete (no matching attachment)";
            return core::makeOk(std::move(result));
        }

        if (matching_attachments.size() > 1) {
            result.is_ambiguous = true;
            for (const auto& [fname, _] : matching_attachments) {
                result.ambiguous_candidates.push_back(fname);
            }
            return core::makeError(core::ErrorCode::METADATA_AMBIGUOUS,
                "PDF contains multiple conflicting metadata attachments",
                {{"candidates", result.ambiguous_candidates}});
        }

        const auto& [fname, payload_str] = matching_attachments[0];
        result.raw_metadata_text = payload_str;

        try {
            result.metadata_payload = nlohmann::json::parse(payload_str);
            result.has_declared_context = true;
            result.notes = "Extracted embedded attachment from PDF: " + fname;
            return core::makeOk(std::move(result));
        } catch (const std::exception& e) {
            return core::makeError(core::ErrorCode::METADATA_JSON_INVALID,
                std::string("Embedded PDF metadata JSON is invalid in ") + fname + ": " + e.what());
        }

    } catch (const std::exception& e) {
        return core::makeError(core::ErrorCode::PDF_ATTACHMENT_ERROR,
            std::string("Failed to process PDF with qpdf: ") + e.what());
    }
}

} // namespace contextlab::ingest
