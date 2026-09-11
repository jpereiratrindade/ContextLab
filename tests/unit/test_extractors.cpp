#include <cassert>
#include <iostream>
#include "contextlab/ingest/MarkdownExtractor.hpp"
#include "contextlab/ingest/LatexExtractor.hpp"
#include "contextlab/ingest/JsonMetadataExtractor.hpp"
#include "contextlab/ingest/TxtExtractor.hpp"

void testMarkdownExtraction() {
    contextlab::ingest::MarkdownExtractor ext;
    std::string md = R"(# Document Title
```context-metadata+json
{
  "document": {
    "id": "DOC-001",
    "title": "Test Title"
  }
}
```
## Section 1
Body text goes here.
)";

    auto res = ext.extract("test.md", md);
    assert(res.has_value());
    assert(res->has_declared_context == true);
    assert(res->metadata_payload["document"]["id"] == "DOC-001");
    assert(res->raw_body_text.find("Body text goes here") != std::string::npos);
    assert(res->raw_body_text.find("```context-metadata+json") == std::string::npos);
    std::cout << "✓ testMarkdownExtraction passed\n";
}

void testLatexExtraction() {
    contextlab::ingest::LatexExtractor ext;
    std::string tex = R"(\documentclass{article}
\begin{filecontents*}{pilot.metadata.json}
{
  "document": {
    "id": "LATEX-001",
    "title": "LaTeX Research Note"
  }
}
\end{filecontents*}
\begin{document}
Hello LaTeX World
\end{document}
)";

    auto res = ext.extract("doc.tex", tex);
    assert(res.has_value());
    assert(res->has_declared_context == true);
    assert(res->metadata_payload["document"]["id"] == "LATEX-001");
    std::cout << "✓ testLatexExtraction passed\n";
}

void testJsonExtraction() {
    contextlab::ingest::JsonMetadataExtractor ext;
    std::string json_str = R"({ "document": { "id": "JSON-001" } })";
    auto res = ext.extract("doc.metadata.json", json_str);
    assert(res.has_value());
    assert(res->has_declared_context == true);
    assert(res->metadata_payload["document"]["id"] == "JSON-001");
    std::cout << "✓ testJsonExtraction passed\n";
}

void testTxtExtraction() {
    contextlab::ingest::TxtExtractor ext;
    std::string txt = "Plain text document content";
    auto res = ext.extract("note.txt", txt);
    assert(res.has_value());
    assert(res->has_declared_context == false);
    assert(res->raw_body_text == txt);
    std::cout << "✓ testTxtExtraction passed\n";
}

int main() {
    std::cout << "Running Extractor Unit Tests...\n";
    testMarkdownExtraction();
    testLatexExtraction();
    testJsonExtraction();
    testTxtExtraction();
    std::cout << "All Extractor Unit Tests PASS!\n";
    return 0;
}
