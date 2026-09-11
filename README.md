# ContextLab

**Laboratório de contexto documental computável e leitura seletiva**

ContextLab é um sistema em C++26 com interface web integrada para investigar e operacionalizar documentos de pesquisa autodescritivos, preservando identidade, autoridade e proveniência, organizando-os prioritariamente por contexto declarado e aprofundando no texto apenas quando a pergunta ou o usuário exige.

Para a especificação formal e fundamentos, consulte [docs/CONTEXTLAB-BOOTSTRAP-001-v0.1.0.md](docs/CONTEXTLAB-BOOTSTRAP-001-v0.1.0.md).

---

## Hipótese Experimental

$$\boxed{
\text{metadados declarados} \longrightarrow \text{contexto suficiente?} \longrightarrow
\begin{cases}
\text{sim} & \longrightarrow \text{organizar / responder sem ler tudo} \\
\text{não} & \longrightarrow \text{aprofundar seletivamente no texto}
\end{cases}
}$$

ContextLab investiga quanto da identidade, status epistemológico, proveniência e relações documentais podem ser resolvidos diretamente a partir dos metadados estruturados autoritativos, eliminando gargalos de processamento desnecessário de CPU/memória.

---

## Como Compilar

Requisitos:
- Compilador C++26 (`g++ >= 14` ou `clang++ >= 18`)
- `CMake >= 3.28` e `Ninja`
- `SQLite3` (com suporte a FTS5)
- `qpdf` (biblioteca e headers para extração de anexos PDF)

```bash
# Configurar e compilar
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j $(nproc)

# Executar suíte de testes
ctest --test-dir build --output-on-failure
```

---

## Como Verificar

Executa a verificação completa dos invariantes e gates do baseline:

```bash
./bin/contextlab verify
```

Saída esperada:
```text
ContextLab verify
────────────────────────────────────────
C++ runtime/build ............. PASS
database migrations ........... PASS
schema registry ............... PASS
self-ingestion ................ PASS
declared metadata authority ... PASS
metadata-only retrieval ....... PASS
selective deepen .............. PASS
PDF attachment path ........... PASS
web/API smoke ................. PASS
────────────────────────────────────────
READY
```

---

## Como Iniciar

Inicializar o repositório, registrar esquemas e autoingerir o documento bootstrap:

```bash
./bin/contextlab init
```

Iniciar o servidor web integrado com a interface visual do ecossistema:

```bash
./bin/contextlab serve --port 8080 --open
```

Acesse no navegador: `http://127.0.0.1:8080`

---

## Comandos CLI

### Ingestão de Documentos
```bash
# Markdown autodescritivo
./bin/contextlab ingest docs/CONTEXTLAB-BOOTSTRAP-001-v0.1.0.md

# PDF com metadados em anexo
./bin/contextlab ingest artigo.pdf

# Arquivo JSON de metadados
./bin/contextlab ingest documento.metadata.json
```

### Consulta e Inspeção
```bash
# Exibir detalhes de um documento pelo ID
./bin/contextlab show CONTEXTLAB-BOOTSTRAP-001

# Listar relações documentais
./bin/contextlab relations CONTEXTLAB-BOOTSTRAP-001

# Listar esquemas registrados
./bin/contextlab schemas

# Estado geral do laboratório
./bin/contextlab status
```

### Busca Inteligente (Metadata-First)
```bash
./bin/contextlab search "documentos do projeto ContextLab"
./bin/contextlab search "motivação da arquitetura metadata-first"
```

### Aprofundamento Textual Seletivo
```bash
./bin/contextlab deepen CONTEXTLAB-BOOTSTRAP-001
```

---

## Onde Ficam os Dados

Os dados locais residem no diretório `data/`:
- `data/contextlab.db`: Banco de dados SQLite contendo metadados estruturados, relações e índice FTS5.
- `data/objects/sha256/xx/xxxx...`: Content-Addressable Storage (CAS) imutável com os artefatos originais ingeridos.

Para apagar o runtime e reiniciar do zero:
```bash
rm -rf data/*.db data/objects
./bin/contextlab init
```

---

## Estado do Laboratório

```text
READY:
  Existe uma baseline completa, verificável e utilizável em C++26
  com persistência, API e interface web.

INCOMPLETE:
  O perfil de contexto, as heurísticas de leitura e a própria arquitetura
  permanecem sujeitos a experimentação e falsificação contínua.
```
