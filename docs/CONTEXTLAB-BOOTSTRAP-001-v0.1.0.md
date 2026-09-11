---
document_id: CONTEXTLAB-BOOTSTRAP-001
title: "ContextLab — base funcional em C++26 com interface web"
version: "0.1.0"
date: "2026-09-11"
language: "pt-BR"
document_type: "implementation_contract"
status: "ready_for_implementation"
primary_project: "ContextLab"
authority: "author_declared"
---

<!--
CONTEXTLAB-METADATA-BEGIN
O bloco JSON abaixo é a autoridade de metadados declarados deste documento.
Um sistema pode indexá-lo sem interpretar o restante do texto.
-->

```context-metadata+json
{
  "schema": {
    "id": "urn:contextlab:document-context",
    "version": "0.1.0",
    "profile": "implementation_contract",
    "authority": "author_declared",
    "serialization": "application/json"
  },
  "document": {
    "id": "CONTEXTLAB-BOOTSTRAP-001",
    "title": "ContextLab — base funcional em C++26 com interface web",
    "subtitle": "Laboratório de contexto documental computável e leitura seletiva",
    "version": "0.1.0",
    "date_created": "2026-09-11",
    "date_modified": "2026-09-11",
    "language": "pt-BR",
    "document_type": "implementation_contract",
    "lifecycle_state": "active",
    "publication_state": "internal_research_and_engineering_document",
    "primary_project": "ContextLab",
    "resource_scope": "bootstrap_complete_baseline",
    "self_describing": true,
    "self_consumption_required": true
  },
  "responsibility": {
    "scientific_and_product_authority": "human_operator",
    "document_authority": "author_declared",
    "implementation_authority": "repository_contract",
    "human_validation_required": true,
    "construction_assistance": [
      {
        "system": "OpenAI ChatGPT",
        "role": "architecture_drafting_revision_and_metadata_structuring",
        "authority": "non_authoritative_assistance"
      }
    ]
  },
  "project_context": {
    "project": "ContextLab",
    "project_kind": "new_independent_laboratory",
    "research_domain": [
      "research_document_context",
      "machine_readable_metadata",
      "provenance",
      "selective_text_analysis",
      "document_retrieval",
      "research_knowledge_infrastructure"
    ],
    "research_stage": "bootstrap",
    "object_of_study": "how much document identity, provenance, epistemic status and relation can be resolved from declared structured context before full-text analysis becomes necessary",
    "central_question": "Can a document system organize, relate and retrieve research artifacts primarily from authoritative embedded metadata and defer textual analysis until the user or the query actually requires it?",
    "engineering_question": "What is the smallest complete C++26 web system that can ingest, validate, organize, relate, search and selectively deepen analysis of self-describing research documents from its first usable execution?"
  },
  "provenance": {
    "origin_context": "research-document metadata pilot conducted while formalizing RES-SAIT-NOTE-001 v0.2.0",
    "origin_context_role": "conceptual_motivation_only",
    "motivating_artifacts": [
      {
        "id": "RES-SAIT-NOTE-001",
        "version": "0.2.0",
        "relation": "motivated_metadata_first_architecture"
      },
      {
        "id": "SAIT-ResearchDocumentContext",
        "version": "0.1.0",
        "relation": "provided_initial_metadata_field_experiment"
      }
    ],
    "identity_boundary": "ContextLab is a new laboratory and system. It does not inherit source code, domain authority, ontology, runtime contracts or product identity from TinyKernel, SisTer or Projeto Resiliência de SAIT.",
    "source_basis": "requirements and design decisions formalized in the research dialogue of 2026-09-11",
    "heritage_policy": "concepts may motivate experiments; implementation starts from a clean repository and defines its own contracts"
  },
  "epistemic_context": {
    "status": "implementation_contract",
    "claim_maturity": "bootstrap_hypothesis",
    "evidence_level": "design_and_prior_metadata_pilot",
    "main_claim": "Declared document metadata should be an authoritative first-pass context layer; full-text analysis should be a conditional deeper operation rather than the default indexing strategy.",
    "not_claimed": [
      "that metadata can replace document text",
      "that all document formats can expose equally reliable metadata",
      "that automated derived metadata has the same authority as author-declared metadata",
      "that an LLM is required for the baseline",
      "that a graph database is required",
      "that this metadata profile is final or universal"
    ],
    "failure_conditions": [
      "the system cannot correctly organize the self-describing bootstrap document without reading its body",
      "declared and derived metadata are silently mixed or overwritten",
      "document identity or lineage requires full-text analysis despite sufficient declared metadata",
      "the first released baseline contains non-functional UI actions, mock endpoints or placeholder persistence",
      "a metadata-only search cannot explain why a result was retrieved"
    ]
  },
  "semantic_context": {
    "primary_concepts": [
      {
        "id": "declared_metadata",
        "label": "metadados declarados",
        "role": "authoritative_first_pass_context"
      },
      {
        "id": "derived_metadata",
        "label": "metadados derivados",
        "role": "system_inference_with_provenance_and_confidence"
      },
      {
        "id": "external_metadata",
        "label": "metadados externos",
        "role": "imported_context_from_external_authority"
      },
      {
        "id": "metadata_first",
        "label": "organização metadata-first",
        "role": "retrieval_strategy"
      },
      {
        "id": "selective_deepening",
        "label": "aprofundamento textual seletivo",
        "role": "deferred_processing_strategy"
      },
      {
        "id": "self_describing_document",
        "label": "documento autodescritivo",
        "role": "primary_research_artifact"
      }
    ],
    "keywords": [
      "C++26",
      "web interface",
      "metadata",
      "provenance",
      "SQLite",
      "PDF attachments",
      "JSON Schema",
      "research documents",
      "retrieval",
      "full text",
      "self ingestion"
    ],
    "distinctions": [
      "declared metadata versus derived metadata",
      "document identity versus document content",
      "metadata retrieval versus text analysis",
      "provenance versus scientific authority",
      "ready baseline versus complete theory"
    ]
  },
  "relations": [
    {
      "subject": "CONTEXTLAB-BOOTSTRAP-001@0.1.0",
      "predicate": "isPartOf",
      "object": "ContextLab"
    },
    {
      "subject": "CONTEXTLAB-BOOTSTRAP-001@0.1.0",
      "predicate": "wasMotivatedBy",
      "object": "RES-SAIT-NOTE-001@0.2.0"
    },
    {
      "subject": "CONTEXTLAB-BOOTSTRAP-001@0.1.0",
      "predicate": "wasMotivatedBy",
      "object": "SAIT-ResearchDocumentContext@0.1.0"
    },
    {
      "subject": "ContextLab",
      "predicate": "doesNotInheritRuntimeFrom",
      "object": "TinyKernel"
    },
    {
      "subject": "ContextLab",
      "predicate": "doesNotInheritRuntimeFrom",
      "object": "SisTer"
    }
  ],
  "quality_and_review": {
    "review_model": "implementation_contract_review",
    "required_gates": [
      "configure",
      "build",
      "unit_tests",
      "integration_tests",
      "self_ingestion",
      "pdf_metadata_ingestion",
      "web_smoke_test",
      "clean_worktree_after_generated_runtime_data_is_ignored"
    ],
    "next_review_trigger": "first complete implementation and usability witness"
  },
  "governance": {
    "confidentiality": "not_classified",
    "sensitivity": "none_declared",
    "license": "not_declared_in_this_bootstrap",
    "allowed_use": "implementation_and_experimental_research",
    "authority_rule": "system-derived context must never overwrite author-declared context",
    "default_network_scope": "localhost_only"
  },
  "computational_processing": {
    "metadata_priority": "authoritative_first_pass",
    "machine_summary": "Implementation contract for a new independent C++26 laboratory named ContextLab. The baseline must be complete and usable from its first execution: ingest self-describing documents, validate metadata, preserve authority layers, persist to SQLite, search metadata first, expose provenance and relations, serve a web UI, and deepen into full text only when explicitly needed.",
    "metadata_only_sufficient_for": [
      "document identity",
      "project grouping",
      "version lineage",
      "provenance navigation",
      "epistemic-status filtering",
      "declared concept filtering",
      "related-artifact ranking",
      "first-pass result explanation"
    ],
    "text_analysis_required_for": [
      "argument evaluation",
      "quotation",
      "claim-evidence assessment",
      "equation or formal content interpretation",
      "contradiction resolution",
      "section-level synthesis",
      "queries explicitly requesting how or why"
    ],
    "deep_text_analysis_triggers": [
      "user explicitly requests deepening",
      "metadata cannot resolve the question",
      "two authoritative metadata sources conflict",
      "the query asks for reasoning contained in the body",
      "the user requests evidence, quotations or detailed synthesis"
    ],
    "do_not_expand_text_when": [
      "metadata fully resolves navigation or filtering",
      "query asks only identity, version, project, provenance, scope or status",
      "the user is browsing document relations"
    ],
    "derived_metadata_policy": "store separately with producer, timestamp, method, source digest and confidence; never overwrite declared metadata",
    "self_consumption": {
      "required": true,
      "bootstrap_document_id": "CONTEXTLAB-BOOTSTRAP-001",
      "acceptance_rule": "after first initialization the system must ingest this Markdown file, validate its JSON metadata and display it in the web interface without requiring body analysis"
    }
  },
  "technical": {
    "implementation_language": "C++26",
    "build_system": "CMake + Ninja",
    "primary_binary": "contextlab",
    "storage": "SQLite with FTS5",
    "web": "embedded HTTP server plus static HTML/CSS/ES modules",
    "api_style": "JSON over HTTP",
    "source_format": "Markdown",
    "metadata_embedding": "fenced context-metadata+json block",
    "encoding": "UTF-8"
  }
}
```

<!-- CONTEXTLAB-METADATA-END -->

# ContextLab

**Laboratório de contexto documental computável e leitura seletiva**

## 1. Mandato

Implementar um sistema novo, independente, em **C++26**, com **interface web**, que funcione integralmente desde sua primeira execução útil.

O sistema deve nascer:

> **pronto, mas incompleto.**

**Pronto** significa que o baseline entregue compila, testa, inicializa, persiste, ingere documentos reais, valida metadados, pesquisa, explica resultados, mostra relações e serve uma interface web funcional.

**Incompleto** significa que nenhuma ontologia, esquema, estratégia de recuperação, formato documental ou mecanismo de análise é tratado como final. O laboratório existe justamente para testar e evoluir essas decisões.

Não implementar uma demo.  
Não implementar uma maquete.  
Não implementar endpoints falsos.  
Não criar botões sem operação real.  
Não duplicar regras de domínio no JavaScript.

O primeiro `main` utilizável deve representar um sistema pequeno, coerente e completo.

---

# 2. Identidade e fronteira

Nome operacional inicial:

```text
ContextLab
```

Executável:

```text
contextlab
```

Subtítulo:

```text
Laboratório de contexto documental computável
```

Este é um **novo laboratório**.

Não reutilizar código, namespaces, banco, schemas de runtime, contratos ou domínio de TinyKernel, SisTer ou Projeto Resiliência de SAIT.

Artefatos anteriores podem ser usados como **dados de teste, motivação ou proveniência**, nunca como herança arquitetural implícita.

O repositório deve poder ser criado vazio e continuar fazendo sentido.

---

# 3. Hipótese experimental do laboratório

A hipótese operacional inicial é:

\[
\boxed{
\text{metadados declarados}
\rightarrow
\text{contexto suficiente?}
\rightarrow
\begin{cases}
\text{sim} & \rightarrow \text{organizar/responder sem ler tudo}\\
\text{não} & \rightarrow \text{aprofundar no texto}
\end{cases}
}
\]

O sistema deve permitir investigar empiricamente:

1. quanto contexto pode ser resolvido sem análise integral;
2. quais campos de metadados realmente ajudam;
3. quais campos tornam-se redundantes;
4. quando a análise textual agrega valor;
5. quanto custa computacionalmente cada estratégia;
6. como relações entre documentos emergem a partir dos metadados;
7. onde inferência automática começa a conflitar com autoridade humana.

O produto não é apenas um catálogo de arquivos.

O produto experimental é a **fronteira entre contexto declarado e interpretação necessária**.
