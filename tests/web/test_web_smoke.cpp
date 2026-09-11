#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <httplib.h>
#include "contextlab/application/ContextLabService.hpp"
#include "contextlab/web/HttpServer.hpp"

int main() {
    std::cout << "Running Web Server Smoke Integration Test...\n";

    std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "contextlab_test_web";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    contextlab::core::AppConfig config;
    config.storage.root = temp_dir / "data";
    config.server.host = "127.0.0.1";
    config.server.port = 18080;

    std::filesystem::path repo_root = std::filesystem::current_path();
    while (!std::filesystem::exists(repo_root / "web")) {
        if (!repo_root.has_parent_path() || repo_root.parent_path() == repo_root) break;
        repo_root = repo_root.parent_path();
    }

    contextlab::application::ContextLabService service(config, repo_root);
    auto init_res = service.initRepository();
    assert(init_res.has_value());

    contextlab::web::HttpServer server(service, repo_root / "web");

    std::thread server_thread([&]() {
        server.listen("127.0.0.1", 18080);
    });

    // Allow server to bind
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Client tests
    httplib::Client client("127.0.0.1", 18080);

    // 1. Root HTML
    auto res_root = client.Get("/");
    assert(res_root && res_root->status == 200);
    assert(res_root->body.find("ContextLab") != std::string::npos);

    // 2. Health endpoint
    auto res_health = client.Get("/api/v1/health");
    assert(res_health && res_health->status == 200);
    assert(res_health->body.find("\"status\": \"ok\"") != std::string::npos ||
           res_health->body.find("\"status\":\"ok\"") != std::string::npos);

    // 3. Documents endpoint
    auto res_docs = client.Get("/api/v1/documents");
    assert(res_docs && res_docs->status == 200);
    assert(res_docs->body.find("CONTEXTLAB-BOOTSTRAP-001") != std::string::npos);

    server.stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }

    std::cout << "✓ Web Server Smoke Test PASS!\n";

    std::filesystem::remove_all(temp_dir);
    return 0;
}
