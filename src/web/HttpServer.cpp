#include "contextlab/web/HttpServer.hpp"
#include "contextlab/core/Logger.hpp"
#include <fstream>
#include <sstream>

namespace contextlab::web {

HttpServer::HttpServer(application::ContextLabService& service, std::filesystem::path web_root)
    : service_(service), web_root_(std::move(web_root)), server_(std::make_unique<httplib::Server>()) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::setupRoutes() {
    // Set CORS and security headers
    server_->set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
        {"X-Content-Type-Options", "nosniff"},
        {"X-Frame-Options", "DENY"}
    });

    server_->Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
    });

    // Root redirect / static serving
    server_->set_mount_point("/", web_root_.string());

    // Helper to resolve authenticated user from Authorization header or token query parameter
    auto extractUser = [this](const httplib::Request& req) -> std::optional<domain::User> {
        std::string token;
        if (req.has_header("Authorization")) {
            std::string auth = req.get_header_value("Authorization");
            if (auth.starts_with("Bearer ")) {
                token = auth.substr(7);
            } else {
                token = auth;
            }
        }
        if (token.empty() && req.has_param("token")) {
            token = req.get_param_value("token");
        }
        if (token.empty()) return std::nullopt;
        auto user_res = service_.authenticateToken(token);
        if (user_res && user_res->has_value()) {
            return *user_res;
        }
        return std::nullopt;
    };

    // Health & System
    auto handle_health = [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(service_.getHealthStatus().dump(2), "application/json");
    };
    server_->Get("/health", handle_health);
    server_->Get("/api/v1/health", handle_health);

    server_->Get("/api/v1/system", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(service_.getSystemInfo().dump(2), "application/json");
    });

    // =========================================================================
    // Auth & User Management (@embrapa.br OTP)
    // =========================================================================
    server_->Post("/api/v1/auth/request-otp", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = nlohmann::json::parse(req.body);
            std::string email = j.value("email", "");
            if (email.empty()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "Campo 'email' é obrigatório"}}).dump(2), "application/json");
                return;
            }
            auto otp_res = service_.requestOtp(email);
            if (!otp_res) {
                res.status = 400;
                res.set_content(otp_res.error().toJson().dump(2), "application/json");
                return;
            }
            res.status = 200;
            res.set_content(nlohmann::json({
                {"success", true},
                {"email", email},
                {"message", "Código de verificação OTP enviado com sucesso para o e-mail informado."},
                {"dev_otp", *otp_res} // Provided for convenient test/dev workflow
            }).dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("JSON inválido: ") + e.what()}}).dump(2), "application/json");
        }
    });

    server_->Post("/api/v1/auth/verify-otp", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = nlohmann::json::parse(req.body);
            std::string email = j.value("email", "");
            std::string code = j.value("code", "");
            if (email.empty() || code.empty()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "Campos 'email' e 'code' são obrigatórios"}}).dump(2), "application/json");
                return;
            }
            auto auth_res = service_.verifyOtp(email, code);
            if (!auth_res) {
                res.status = 401;
                res.set_content(auth_res.error().toJson().dump(2), "application/json");
                return;
            }
            const auto& [user, token] = *auth_res;
            res.status = 200;
            res.set_content(nlohmann::json({
                {"success", true},
                {"token", token},
                {"user", user.toJson()}
            }).dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("JSON inválido: ") + e.what()}}).dump(2), "application/json");
        }
    });

    server_->Get("/api/v1/auth/me", [this, extractUser](const httplib::Request& req, httplib::Response& res) {
        auto user = extractUser(req);
        if (user.has_value()) {
            res.status = 200;
            res.set_content(nlohmann::json({
                {"authenticated", true},
                {"user", user->toJson()}
            }).dump(2), "application/json");
        } else {
            res.status = 200;
            res.set_content(nlohmann::json({
                {"authenticated", false},
                {"user", nullptr}
            }).dump(2), "application/json");
        }
    });

    server_->Post("/api/v1/auth/logout", [this](const httplib::Request& req, httplib::Response& res) {
        std::string token;
        if (req.has_header("Authorization")) {
            std::string auth = req.get_header_value("Authorization");
            if (auth.starts_with("Bearer ")) token = auth.substr(7);
            else token = auth;
        }
        if (!token.empty()) {
            (void)service_.logout(token);
        }
        res.status = 200;
        res.set_content(nlohmann::json({{"success", true}}).dump(2), "application/json");
    });

    // Ingest endpoint (supports JSON or multipart upload)
    server_->Post("/api/v1/ingest", [this](const httplib::Request& req, httplib::Response& res) {
        if (req.has_file("file")) {
            const auto& file = req.get_file_value("file");
            auto report_res = service_.ingestContent(file.content, file.filename, file.content_type);
            if (!report_res) {
                res.status = 400;
                res.set_content(report_res.error().toJson().dump(2), "application/json");
                return;
            }
            res.status = 200;
            res.set_content(report_res->toJson().dump(2), "application/json");
            return;
        }

        try {
            auto j = nlohmann::json::parse(req.body);
            if (j.contains("filepath") && j["filepath"].is_string()) {
                auto path = j["filepath"].get<std::string>();
                auto report_res = service_.ingestFile(path);
                if (!report_res) {
                    res.status = 400;
                    res.set_content(report_res.error().toJson().dump(2), "application/json");
                    return;
                }
                res.status = 200;
                res.set_content(report_res->toJson().dump(2), "application/json");
                return;
            }

            if (j.contains("content") && j["content"].is_string()) {
                auto content = j["content"].get<std::string>();
                std::string fname = j.value("filename", "upload.md");
                std::string mtype = j.value("media_type", "");
                auto report_res = service_.ingestContent(content, fname, mtype);
                if (!report_res) {
                    res.status = 400;
                    res.set_content(report_res.error().toJson().dump(2), "application/json");
                    return;
                }
                res.status = 200;
                res.set_content(report_res->toJson().dump(2), "application/json");
                return;
            }

            res.status = 400;
            res.set_content(nlohmann::json({{"error", "Request must contain 'file' multipart or 'filepath'/'content' in JSON"}}).dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("Invalid JSON body: ") + e.what()}}).dump(2), "application/json");
        }
    });

    // Documents (Access-controlled)
    server_->Get("/api/v1/documents", [this, extractUser](const httplib::Request& req, httplib::Response& res) {
        auto user = extractUser(req);
        auto docs_res = service_.listDocuments(user);
        if (!docs_res) {
            res.status = 500;
            res.set_content(docs_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& d : *docs_res) arr.push_back(d.toJson());
        res.set_content(arr.dump(2), "application/json");
    });

    server_->Get(R"(/api/v1/documents/([^/]+))", [this, extractUser](const httplib::Request& req, httplib::Response& res) {
        std::string doc_id = req.matches[1];
        auto user = extractUser(req);
        auto doc_res = service_.getDocumentFull(doc_id, user);
        if (!doc_res) {
            int code = 500;
            if (doc_res.error().code == core::ErrorCode::FILE_NOT_FOUND) code = 404;
            else if (doc_res.error().code == core::ErrorCode::PERMISSION_DENIED) code = 403;
            res.status = code;
            res.set_content(doc_res.error().toJson().dump(2), "application/json");
            return;
        }
        res.set_content(doc_res->dump(2), "application/json");
    });

    server_->Put(R"(/api/v1/documents/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string doc_id = req.matches[1];
            auto existing = service_.getDocument(doc_id);
            if (!existing) {
                res.status = 404;
                res.set_content(existing.error().toJson().dump(2), "application/json");
                return;
            }
            auto j = nlohmann::json::parse(req.body);
            auto doc = *existing;
            if (j.contains("title") && j["title"].is_string()) doc.title = j["title"].get<std::string>();
            if (j.contains("subtitle") && j["subtitle"].is_string()) doc.subtitle = j["subtitle"].get<std::string>();
            if (j.contains("version") && j["version"].is_string()) doc.version = j["version"].get<std::string>();
            if (j.contains("language") && j["language"].is_string()) doc.language = j["language"].get<std::string>();
            if (j.contains("document_type") && j["document_type"].is_string()) doc.document_type = j["document_type"].get<std::string>();
            if (j.contains("lifecycle_state") && j["lifecycle_state"].is_string()) doc.lifecycle_state = j["lifecycle_state"].get<std::string>();
            if (j.contains("publication_state") && j["publication_state"].is_string()) doc.publication_state = j["publication_state"].get<std::string>();
            if (j.contains("primary_project") && j["primary_project"].is_string()) doc.primary_project = j["primary_project"].get<std::string>();
            if (j.contains("resource_scope") && j["resource_scope"].is_string()) doc.resource_scope = j["resource_scope"].get<std::string>();
            if (j.contains("epistemic_status") && j["epistemic_status"].is_string()) doc.epistemic_status = j["epistemic_status"].get<std::string>();

            const auto now = std::chrono::system_clock::now();
            doc.date_modified = std::format("{:%Y-%m-%d %H:%M:%S}", now);

            auto update_res = service_.updateDocument(doc);
            if (!update_res) {
                res.status = 500;
                res.set_content(update_res.error().toJson().dump(2), "application/json");
                return;
            }
            res.set_content(doc.toJson().dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("Invalid JSON: ") + e.what()}}).dump(2), "application/json");
        }
    });

    server_->Delete(R"(/api/v1/documents/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        std::string doc_id = req.matches[1];
        auto del_res = service_.deleteDocument(doc_id);
        if (!del_res) {
            res.status = 500;
            res.set_content(del_res.error().toJson().dump(2), "application/json");
            return;
        }
        res.set_content(nlohmann::json({{"success", true}, {"deleted_id", doc_id}}).dump(2), "application/json");
    });

    server_->Get(R"(/api/v1/documents/([^/]+)/metadata)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string doc_id = req.matches[1];
        auto doc_res = service_.getDocumentFull(doc_id);
        if (!doc_res) {
            res.status = 404;
            res.set_content(doc_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json resp = {
            {"document_id", doc_id},
            {"metadata_envelopes", (*doc_res)["metadata_envelopes"]}
        };
        res.set_content(resp.dump(2), "application/json");
    });

    server_->Delete(R"(/api/v1/metadata/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        std::string env_id = req.matches[1];
        auto del_res = service_.deleteMetadataEnvelope(env_id);
        if (!del_res) {
            res.status = 500;
            res.set_content(del_res.error().toJson().dump(2), "application/json");
            return;
        }
        res.set_content(nlohmann::json({{"success", true}, {"deleted_id", env_id}}).dump(2), "application/json");
    });

    server_->Get(R"(/api/v1/documents/([^/]+)/relations)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string doc_id = req.matches[1];
        auto doc_res = service_.getDocumentFull(doc_id);
        if (!doc_res) {
            res.status = 404;
            res.set_content(doc_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json resp = {
            {"document_id", doc_id},
            {"relations", (*doc_res)["relations"]}
        };
        res.set_content(resp.dump(2), "application/json");
    });

    server_->Get(R"(/api/v1/documents/([^/]+)/artifacts)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string doc_id = req.matches[1];
        auto doc_res = service_.getDocumentFull(doc_id);
        if (!doc_res) {
            res.status = 404;
            res.set_content(doc_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json resp = {
            {"document_id", doc_id},
            {"artifacts", (*doc_res)["artifacts"]}
        };
        res.set_content(resp.dump(2), "application/json");
    });

    server_->Post(R"(/api/v1/documents/([^/]+)/deepen)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string doc_id = req.matches[1];
        auto deep_res = service_.deepen(doc_id);
        if (!deep_res) {
            res.status = (deep_res.error().code == core::ErrorCode::TEXT_EXTRACTION_UNAVAILABLE) ? 422 : 500;
            res.set_content(deep_res.error().toJson().dump(2), "application/json");
            return;
        }
        res.set_content(deep_res->toJson().dump(2), "application/json");
    });

    server_->Get(R"(/api/v1/documents/([^/]+)/text)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string doc_id = req.matches[1];
        auto doc_res = service_.getDocumentFull(doc_id);
        if (!doc_res) {
            res.status = 404;
            res.set_content(doc_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json resp = {
            {"document_id", doc_id},
            {"text_analysis", (*doc_res)["text_analysis"]}
        };
        res.set_content(resp.dump(2), "application/json");
    });

    // Projects
    server_->Get("/api/v1/projects", [this](const httplib::Request&, httplib::Response& res) {
        auto projs_res = service_.listProjects();
        if (!projs_res) {
            res.status = 500;
            res.set_content(projs_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& p : *projs_res) arr.push_back(p.toJson());
        res.set_content(arr.dump(2), "application/json");
    });

    server_->Post("/api/v1/projects", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = nlohmann::json::parse(req.body);
            std::string p_id = j.value("id", "");
            if (p_id.empty()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "Project 'id' is required"}}).dump(2), "application/json");
                return;
            }
            std::vector<std::string> domains;
            if (j.contains("research_domain") && j["research_domain"].is_array()) {
                domains = j["research_domain"].get<std::vector<std::string>>();
            }
            domain::Project p{
                .id = p_id,
                .name = j.value("name", p_id),
                .kind = j.value("kind", "research_project"),
                .research_domain = std::move(domains),
                .stage = j.value("stage", "active"),
                .object_of_study = j.value("object_of_study", ""),
                .central_question = j.value("central_question", ""),
                .engineering_question = j.value("engineering_question", "")
            };
            auto create_res = service_.createProject(p);
            if (!create_res) {
                res.status = 500;
                res.set_content(create_res.error().toJson().dump(2), "application/json");
                return;
            }
            res.status = 201;
            res.set_content(p.toJson().dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("Invalid JSON: ") + e.what()}}).dump(2), "application/json");
        }
    });

    server_->Put(R"(/api/v1/projects/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string p_id = req.matches[1];
            auto j = nlohmann::json::parse(req.body);
            std::vector<std::string> domains;
            if (j.contains("research_domain") && j["research_domain"].is_array()) {
                domains = j["research_domain"].get<std::vector<std::string>>();
            }
            domain::Project p{
                .id = p_id,
                .name = j.value("name", p_id),
                .kind = j.value("kind", "research_project"),
                .research_domain = std::move(domains),
                .stage = j.value("stage", "active"),
                .object_of_study = j.value("object_of_study", ""),
                .central_question = j.value("central_question", ""),
                .engineering_question = j.value("engineering_question", "")
            };
            auto update_res = service_.updateProject(p);
            if (!update_res) {
                res.status = 500;
                res.set_content(update_res.error().toJson().dump(2), "application/json");
                return;
            }
            res.set_content(p.toJson().dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("Invalid JSON: ") + e.what()}}).dump(2), "application/json");
        }
    });

    server_->Delete(R"(/api/v1/projects/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        std::string p_id = req.matches[1];
        auto del_res = service_.deleteProject(p_id);
        if (!del_res) {
            res.status = 500;
            res.set_content(del_res.error().toJson().dump(2), "application/json");
            return;
        }
        res.set_content(nlohmann::json({{"success", true}, {"deleted_id", p_id}}).dump(2), "application/json");
    });

    // Relations
    server_->Get("/api/v1/relations", [this](const httplib::Request&, httplib::Response& res) {
        auto rels_res = service_.listRelations();
        if (!rels_res) {
            res.status = 500;
            res.set_content(rels_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& r : *rels_res) arr.push_back(r.toJson());
        res.set_content(arr.dump(2), "application/json");
    });

    server_->Post("/api/v1/relations", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = nlohmann::json::parse(req.body);
            std::string sub = j.value("subject", "");
            std::string pred = j.value("predicate", "");
            std::string obj = j.value("object", "");
            std::string doc_id = j.value("document_id", "");
            if (sub.empty() || pred.empty() || obj.empty()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", "'subject', 'predicate' and 'object' are required"}}).dump(2), "application/json");
                return;
            }
            domain::Relation r{
                .id = 0,
                .subject = std::move(sub),
                .predicate = std::move(pred),
                .object = std::move(obj),
                .document_id = std::move(doc_id)
            };
            auto save_res = service_.createRelation(r);
            if (!save_res) {
                res.status = 500;
                res.set_content(save_res.error().toJson().dump(2), "application/json");
                return;
            }
            res.status = 201;
            res.set_content(r.toJson().dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("Invalid JSON: ") + e.what()}}).dump(2), "application/json");
        }
    });

    server_->Put(R"(/api/v1/relations/([0-9]+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            int64_t rel_id = std::stoll(req.matches[1]);
            auto j = nlohmann::json::parse(req.body);
            domain::Relation r{
                .id = rel_id,
                .subject = j.value("subject", ""),
                .predicate = j.value("predicate", ""),
                .object = j.value("object", ""),
                .document_id = j.value("document_id", "")
            };
            auto update_res = service_.updateRelation(r);
            if (!update_res) {
                res.status = 500;
                res.set_content(update_res.error().toJson().dump(2), "application/json");
                return;
            }
            res.set_content(r.toJson().dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("Invalid JSON: ") + e.what()}}).dump(2), "application/json");
        }
    });

    server_->Delete(R"(/api/v1/relations/([0-9]+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            int64_t rel_id = std::stoll(req.matches[1]);
            auto del_res = service_.deleteRelation(rel_id);
            if (!del_res) {
                res.status = 500;
                res.set_content(del_res.error().toJson().dump(2), "application/json");
                return;
            }
            res.set_content(nlohmann::json({{"success", true}, {"deleted_id", rel_id}}).dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("Invalid relation ID: ") + e.what()}}).dump(2), "application/json");
        }
    });

    // Schemas
    server_->Get("/api/v1/schemas", [this](const httplib::Request&, httplib::Response& res) {
        auto schemas_res = service_.listSchemas();
        if (!schemas_res) {
            res.status = 500;
            res.set_content(schemas_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& s : *schemas_res) arr.push_back(s.toJson());
        res.set_content(arr.dump(2), "application/json");
    });

    // Search (Access-controlled)
    server_->Post("/api/v1/search", [this, extractUser](const httplib::Request& req, httplib::Response& res) {
        try {
            auto user = extractUser(req);
            auto j = nlohmann::json::parse(req.body);
            std::string query = j.value("query", "");
            std::string mode = j.value("mode", "auto");

            auto search_res = service_.search(query, mode, user);
            if (!search_res) {
                res.status = 500;
                res.set_content(search_res.error().toJson().dump(2), "application/json");
                return;
            }
            res.set_content(search_res->toJson().dump(2), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", std::string("Invalid JSON body: ") + e.what()}}).dump(2), "application/json");
        }
    });

    // Topic Graph & Epistemic Constellation Analytics
    server_->Get("/api/v1/analytics/topic-graph", [this, extractUser](const httplib::Request& req, httplib::Response& res) {
        auto user = extractUser(req);
        auto graph_res = service_.getTopicGraph(user);
        if (!graph_res) {
            res.status = 500;
            res.set_content(graph_res.error().toJson().dump(2), "application/json");
            return;
        }
        res.set_content(graph_res->toJson().dump(2), "application/json");
    });

    // Events
    server_->Get("/api/v1/events", [this](const httplib::Request&, httplib::Response& res) {
        auto evs_res = service_.getRecentEvents(50);
        if (!evs_res) {
            res.status = 500;
            res.set_content(evs_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& ev : *evs_res) arr.push_back(ev.toJson());
        res.set_content(arr.dump(2), "application/json");
    });
}

bool HttpServer::listen(const std::string& host, int port) {
    setupRoutes();
    core::Logger::instance().info("SERVER_STARTING", "HTTP Server listening on http://" + host + ":" + std::to_string(port));
    return server_->listen(host.c_str(), port);
}

void HttpServer::stop() {
    if (server_ && server_->is_running()) {
        server_->stop();
    }
}

} // namespace contextlab::web
