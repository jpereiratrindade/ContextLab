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

    // Health & System
    auto handle_health = [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(service_.getHealthStatus().dump(2), "application/json");
    };
    server_->Get("/health", handle_health);
    server_->Get("/api/v1/health", handle_health);

    server_->Get("/api/v1/system", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(service_.getSystemInfo().dump(2), "application/json");
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

    // Documents
    server_->Get("/api/v1/documents", [this](const httplib::Request&, httplib::Response& res) {
        auto docs_res = service_.listDocuments();
        if (!docs_res) {
            res.status = 500;
            res.set_content(docs_res.error().toJson().dump(2), "application/json");
            return;
        }
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& d : *docs_res) arr.push_back(d.toJson());
        res.set_content(arr.dump(2), "application/json");
    });

    server_->Get(R"(/api/v1/documents/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        std::string doc_id = req.matches[1];
        auto doc_res = service_.getDocumentFull(doc_id);
        if (!doc_res) {
            res.status = (doc_res.error().code == core::ErrorCode::FILE_NOT_FOUND) ? 404 : 500;
            res.set_content(doc_res.error().toJson().dump(2), "application/json");
            return;
        }
        res.set_content(doc_res->dump(2), "application/json");
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

    // Search
    server_->Post("/api/v1/search", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = nlohmann::json::parse(req.body);
            std::string query = j.value("query", "");
            std::string mode = j.value("mode", "auto");

            auto search_res = service_.search(query, mode);
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
