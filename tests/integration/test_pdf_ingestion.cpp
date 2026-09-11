#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "contextlab/application/ContextLabService.hpp"
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFWriter.hh>
#include <qpdf/QPDFFileSpecObjectHelper.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFEmbeddedFileDocumentHelper.hh>

int main() {
    std::cout << "Running PDF Metadata Ingestion Integration Test...\n";

    std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "contextlab_test_pdf";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    std::filesystem::path pdf_path = temp_dir / "sample_pilot.pdf";
    std::filesystem::path meta_path = temp_dir / "RES-SAIT-NOTE-001.metadata.json";

    std::string metadata_json = R"({
        "schema": {
            "id": "urn:contextlab:document-context",
            "version": "0.1.0",
            "authority": "author_declared"
        },
        "document": {
            "id": "RES-SAIT-NOTE-001",
            "title": "Relação Triádica EPS em Engenharia de Software",
            "version": "0.2.0",
            "date_created": "2026-09-10",
            "language": "pt-BR",
            "document_type": "research_note",
            "primary_project": "Projeto Resiliência de SAIT"
        },
        "responsibility": {},
        "project_context": {
            "project": "Projeto Resiliência de SAIT",
            "object_of_study": "Resilience in SAIT",
            "central_question": "How to model triadic EPS relations?"
        },
        "provenance": {
            "origin_context": "TinyKernel",
            "origin_context_role": "context_of_genesis_and_methodological_stimulation_only",
            "identity_boundary": "Independent note",
            "source_basis": "Empirical data"
        },
        "epistemic_context": {
            "status": "working_hypothesis",
            "main_claim": "EPS triadic relations model resilience dynamics",
            "not_claimed": ["final theory"]
        },
        "semantic_context": {
            "primary_concepts": [{"id": "eps", "label": "EPS", "role": "concept"}],
            "keywords": ["resilience", "SAIT"]
        },
        "computational_processing": {
            "metadata_priority": "authoritative_first_pass",
            "metadata_only_sufficient_for": ["identity", "project"],
            "text_analysis_required_for": ["deep synthesis"],
            "derived_metadata_policy": "never overwrite declared"
        },
        "technical": {
            "implementation_language": "C++26",
            "encoding": "UTF-8"
        }
    })";

    {
        std::ofstream mf(meta_path);
        mf << metadata_json;
    }

    // 1. Create a minimal PDF with an embedded metadata.json attachment using QPDF
    {
        QPDF qpdf;
        qpdf.emptyPDF();
        
        // Add a blank page
        auto page = QPDFObjectHandle::parse(
            "<< /Type /Page /MediaBox [0 0 612 792] >>"
        );
        QPDFPageDocumentHelper(qpdf).addPage(page, false);

        auto fs = QPDFFileSpecObjectHelper::createFileSpec(qpdf, "RES-SAIT-NOTE-001.metadata.json", meta_path.string());
        
        QPDFEmbeddedFileDocumentHelper efdh(qpdf);
        efdh.replaceEmbeddedFile("RES-SAIT-NOTE-001.metadata.json", fs);

        QPDFWriter writer(qpdf, pdf_path.string().c_str());
        writer.write();
    }

    assert(std::filesystem::exists(pdf_path));

    // 2. Ingest PDF into ContextLab
    contextlab::core::AppConfig config;
    config.storage.root = temp_dir / "data";

    std::filesystem::path repo_root = std::filesystem::current_path();
    while (!std::filesystem::exists(repo_root / "schemas")) {
        if (!repo_root.has_parent_path() || repo_root.parent_path() == repo_root) break;
        repo_root = repo_root.parent_path();
    }

    contextlab::application::ContextLabService service(config, repo_root);
    auto init_res = service.initialize();
    assert(init_res.has_value());

    auto ingest_res = service.ingestFile(pdf_path);
    assert(ingest_res.has_value());
    assert(ingest_res->success == true);
    assert(ingest_res->document_id == "RES-SAIT-NOTE-001");
    assert(ingest_res->has_declared_context == true);
    assert(ingest_res->text_analysis_performed == false);

    auto doc_res = service.getDocument("RES-SAIT-NOTE-001");
    assert(doc_res.has_value());
    assert(doc_res->primary_project == "Projeto Resiliência de SAIT");
    assert(doc_res->text_analyzed == false);

    std::cout << "✓ PDF Metadata Ingestion Test PASS!\n";

    std::filesystem::remove_all(temp_dir);
    return 0;
}
