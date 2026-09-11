#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <httplib.h>
#include "contextlab/application/ContextLabService.hpp"

namespace contextlab::web {

class HttpServer {
public:
    HttpServer(application::ContextLabService& service, std::filesystem::path web_root);
    ~HttpServer();

    void setupRoutes();
    bool listen(const std::string& host, int port);
    void stop();

private:
    application::ContextLabService& service_;
    std::filesystem::path web_root_;
    std::unique_ptr<httplib::Server> server_;
};

} // namespace contextlab::web
