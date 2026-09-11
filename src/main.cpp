#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <cstdlib>
#include "contextlab/application/ContextLabService.hpp"
#include "contextlab/web/HttpServer.hpp"
#include "contextlab/core/Logger.hpp"

namespace {

void printHelp() {
    std::cout << R"(ContextLab — Laboratório de contexto documental computável e leitura seletiva

Uso:
  contextlab <comando> [opções] [argumentos]

Comandos:
  verify                    Executa a suíte de verificação completa do baseline
  init                      Inicializa banco de dados, registra schemas e autoingere o bootstrap
  serve [--port N] [--open] Inicia o servidor HTTP integrado e serve a interface web
  ingest <arquivos...>      Ingere um ou mais documentos (Markdown, PDF, JSON, LaTeX, TXT)
  show <doc_id>             Exibe metadados, integridade e contexto de um documento
  search <query> [--mode M] Busca metadata-first com explicação de rota (auto|metadata|text)
  deepen <doc_id>           Executa aprofundamento textual seletivo no documento
  relations [doc_id]        Lista as relações registradas no corpus ou para um documento
  schemas                   Lista os esquemas de metadados registrados
  status                    Exibe estatísticas de persistência e integridade do laboratório
  help                      Exibe esta mensagem de ajuda
)";
}

std::filesystem::path findRepoRoot() {
    auto current = std::filesystem::current_path();
    while (true) {
        if (std::filesystem::exists(current / "docs" / "CONTEXTLAB-BOOTSTRAP-001-v0.1.0.md") ||
            std::filesystem::exists(current / "schemas" / "contextlab-document-context-v0.1.0.schema.json")) {
            return current;
        }
        if (!current.has_parent_path() || current.parent_path() == current) {
            break;
        }
        current = current.parent_path();
    }
    return std::filesystem::current_path();
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    std::string command = argv[1];
    if (command == "help" || command == "--help" || command == "-h") {
        printHelp();
        return 0;
    }

    auto repo_root = findRepoRoot();
    auto config_path = repo_root / "data" / "contextlab.json";
    auto config = contextlab::core::AppConfig::loadFromFile(config_path);

    // Make storage paths absolute relative to repo_root if relative
    if (config.storage.root.is_relative()) {
        config.storage.root = repo_root / config.storage.root;
    }

    contextlab::application::ContextLabService service(config, repo_root);

    if (command == "verify") {
        contextlab::core::Logger::instance().setMinLevel(contextlab::core::LogLevel::ERROR);
        std::cout << "ContextLab verify\n";
        std::cout << "────────────────────────────────────────\n";
        auto report = service.runVerification();

        for (const auto& gate : report.gates) {
            std::string dots(31 > gate.name.size() ? 31 - gate.name.size() : 2, '.');
            std::cout << gate.name << " " << dots << " "
                      << (gate.passed ? "PASS" : "FAIL") << "\n";
        }
        std::cout << "────────────────────────────────────────\n";
        if (report.all_passed) {
            std::cout << "READY\n";
            return 0;
        } else {
            std::cout << "FAILED GATES DETECTED\n";
            return 1;
        }
    }

    if (command == "init") {
        std::cout << "Inicializando ContextLab em: " << repo_root.string() << "\n";
        auto res = service.initRepository();
        if (!res) {
            std::cerr << "Erro ao inicializar: " << res.error().message << "\n";
            return 1;
        }
        std::cout << "✓ Diretórios de armazenamento e banco SQLite inicializados.\n";
        std::cout << "✓ Esquemas registrados com sucesso.\n";
        std::cout << "✓ Documento de bootstrap CONTEXTLAB-BOOTSTRAP-001 autoingerido.\n";
        std::cout << "ContextLab está pronto. Execute: ./bin/contextlab serve --open\n";
        return 0;
    }

    // For other commands, ensure DB & schemas are initialized
    auto init_res = service.initialize();
    if (!init_res) {
        std::cerr << "Erro ao carregar banco: " << init_res.error().message << "\n";
        return 1;
    }

    if (command == "serve") {
        int port = config.server.port;
        std::string host = config.server.host;
        bool open_browser = false;

        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--port" && i + 1 < argc) {
                port = std::stoi(argv[++i]);
            } else if (arg == "--host" && i + 1 < argc) {
                host = argv[++i];
            } else if (arg == "--open") {
                open_browser = true;
            }
        }

        auto web_root = repo_root / "web";
        contextlab::web::HttpServer server(service, web_root);

        std::cout << "ContextLab Server\n";
        std::cout << "Interface Web: http://" << host << ":" << port << "\n";
        std::cout << "API v1:        http://" << host << ":" << port << "/api/v1\n";
        std::cout << "Pressione Ctrl+C para encerrar.\n";

        if (open_browser) {
            std::string url = "http://" + host + ":" + std::to_string(port);
#if defined(__APPLE__)
            (void)std::system(("open " + url).c_str());
#elif defined(__linux__)
            (void)std::system(("xdg-open " + url + " >/dev/null 2>&1 &").c_str());
#endif
        }

        if (!server.listen(host, port)) {
            std::cerr << "Falha ao iniciar servidor HTTP na porta " << port << "\n";
            return 1;
        }
        return 0;
    }

    if (command == "ingest") {
        if (argc < 3) {
            std::cerr << "Uso: contextlab ingest <arquivo1> [arquivo2...]\n";
            return 1;
        }

        for (int i = 2; i < argc; ++i) {
            std::filesystem::path p = argv[i];
            std::cout << "Ingerindo: " << p.string() << " ... ";
            auto res = service.ingestFile(p);
            if (!res) {
                std::cout << "\033[31mFALHA\033[0m: " << res.error().message << "\n";
            } else {
                std::cout << "\033[32mOK\033[0m (Document ID: " << res->document_id
                          << ", Formato: " << res->format_detected
                          << ", Autoridade declarada: " << (res->has_declared_context ? "SIM" : "NÃO")
                          << ")\n";
            }
        }
        return 0;
    }

    if (command == "show") {
        if (argc < 3) {
            std::cerr << "Uso: contextlab show <document_id>\n";
            return 1;
        }
        std::string doc_id = argv[2];
        auto res = service.getDocumentFull(doc_id);
        if (!res) {
            std::cerr << "Erro: " << res.error().message << "\n";
            return 1;
        }
        std::cout << res->dump(2) << "\n";
        return 0;
    }

    if (command == "search") {
        if (argc < 3) {
            std::cerr << "Uso: contextlab search <query> [--mode auto|metadata|text]\n";
            return 1;
        }
        std::string query = argv[2];
        std::string mode = "auto";
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--mode" && i + 1 < argc) {
                mode = argv[++i];
            }
        }

        auto res = service.search(query, mode);
        if (!res) {
            std::cerr << "Erro na busca: " << res.error().message << "\n";
            return 1;
        }
        std::cout << res->toJson().dump(2) << "\n";
        return 0;
    }

    if (command == "deepen") {
        if (argc < 3) {
            std::cerr << "Uso: contextlab deepen <document_id>\n";
            return 1;
        }
        std::string doc_id = argv[2];
        std::cout << "Executando aprofundamento textual seletivo para: " << doc_id << " ...\n";
        auto res = service.deepen(doc_id);
        if (!res) {
            std::cerr << "\033[31mErro no aprofundamento:\033[0m " << res.error().message << "\n";
            return 1;
        }
        std::cout << "\033[32mAprofundamento concluído com sucesso!\033[0m\n";
        std::cout << res->toJson().dump(2) << "\n";
        return 0;
    }

    if (command == "relations") {
        auto res = service.listRelations();
        if (!res) {
            std::cerr << "Erro ao listar relações: " << res.error().message << "\n";
            return 1;
        }
        std::string filter_doc = (argc >= 3) ? argv[2] : "";
        std::cout << "Relações no Corpus (" << res->size() << " total):\n";
        for (const auto& r : *res) {
            if (filter_doc.empty() || r.document_id == filter_doc) {
                std::cout << "  [" << r.document_id << "] " << r.subject << " --(" << r.predicate << ")--> " << r.object << "\n";
            }
        }
        return 0;
    }

    if (command == "schemas") {
        auto res = service.listSchemas();
        if (!res) {
            std::cerr << "Erro ao listar schemas: " << res.error().message << "\n";
            return 1;
        }
        std::cout << "Esquemas registrados (" << res->size() << "):\n";
        for (const auto& s : *res) {
            std::cout << "  • ID: " << s.id << " (v" << s.version << ") - " << s.source_path << "\n";
        }
        return 0;
    }

    if (command == "status") {
        std::cout << service.getSystemInfo().dump(2) << "\n";
        return 0;
    }

    std::cerr << "Comando desconhecido: " << command << "\n";
    printHelp();
    return 1;
}
