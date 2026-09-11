#include "contextlab/application/ContextLabService.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>

using namespace contextlab;

int main() {
    std::cout << "Running test_auth_and_visibility..." << std::endl;

    auto temp_dir = std::filesystem::temp_directory_path() / "contextlab_test_auth_vis";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    core::AppConfig config;
    config.storage.root = temp_dir / "data";

    std::filesystem::path repo_root = std::filesystem::current_path();
    while (!std::filesystem::exists(repo_root / "schemas")) {
        if (!repo_root.has_parent_path() || repo_root.parent_path() == repo_root) break;
        repo_root = repo_root.parent_path();
    }

    application::ContextLabService service(config, repo_root);
    auto init_res = service.initialize();
    assert(init_res.has_value());

    // 1. Rejection of non-embrapa emails
    auto inv_otp = service.requestOtp("usuario@gmail.com");
    assert(!inv_otp.has_value());
    assert(inv_otp.error().code == core::ErrorCode::INVALID_ARGUMENT);

    // 2. Request OTP for @embrapa.br email
    std::string email = "joao.silva@embrapa.br";
    auto otp_res = service.requestOtp(email);
    assert(otp_res.has_value());
    std::string otp = *otp_res;
    assert(otp.length() == 6);

    // 3. Verification with wrong OTP fails
    auto fail_verify = service.verifyOtp(email, "000000");
    assert(!fail_verify.has_value());
    assert(fail_verify.error().code == core::ErrorCode::PERMISSION_DENIED);

    // 4. Verification with correct OTP succeeds
    auto verify_res = service.verifyOtp(email, otp);
    assert(verify_res.has_value());
    auto [user, token] = *verify_res;
    assert(user.email == email);
    assert(user.name == "Joao Silva");
    assert(!token.empty());

    // 5. Authenticate token
    auto auth_user = service.authenticateToken(token);
    assert(auth_user.has_value() && auth_user->has_value());
    assert((*auth_user)->email == email);

    // 6. Test Document Visibility Filters
    domain::Document pub_doc{
        .id = "DOC-PUB-001",
        .title = "Balanço Geral da Agropecuária",
        .version = "1.0.0",
        .visibility = "public"
    };
    domain::Document embrapa_doc{
        .id = "DOC-EMBRAPA-002",
        .title = "Diretrizes Internas de Pesquisa ILPF",
        .version = "1.0.0",
        .visibility = "internal_embrapa"
    };
    domain::Document priv_doc{
        .id = "DOC-PRIV-003",
        .title = "Rascunho de Patente Confidencial",
        .version = "0.1.0",
        .owner = "maria.santos@embrapa.br",
        .visibility = "private"
    };

    assert(service.updateDocument(pub_doc).has_value());
    assert(service.updateDocument(embrapa_doc).has_value());
    assert(service.updateDocument(priv_doc).has_value());

    // Guest view: only public
    auto guest_docs = service.listDocuments(std::nullopt);
    assert(guest_docs.has_value());
    assert(guest_docs->size() == 1);
    assert((*guest_docs)[0].id == "DOC-PUB-001");

    // Guest access to private document fails
    auto guest_priv_get = service.getDocument("DOC-PRIV-003", std::nullopt);
    assert(!guest_priv_get.has_value());
    assert(guest_priv_get.error().code == core::ErrorCode::PERMISSION_DENIED);

    // Logged in user view: sees public + internal_embrapa, but not other's private
    auto user_docs = service.listDocuments(user);
    assert(user_docs.has_value());
    assert(user_docs->size() == 2);

    // Own private doc created by user
    domain::Document my_priv_doc{
        .id = "DOC-MY-PRIV-004",
        .title = "Minhas Anotações de Campo",
        .version = "0.1.0",
        .owner = email,
        .visibility = "private"
    };
    assert(service.updateDocument(my_priv_doc).has_value());

    auto my_docs = service.listDocuments(user);
    assert(my_docs.has_value());
    assert(my_docs->size() == 3); // pub, internal, own private

    // 7. Topic Graph Analytics
    auto graph_res = service.getTopicGraph(user);
    assert(graph_res.has_value());
    assert(!graph_res->nodes.empty());
    assert(!graph_res->edges.empty());
    assert(graph_res->total_documents >= 3);

    // Clean up
    std::filesystem::remove_all(temp_dir);
    std::cout << "test_auth_and_visibility PASSED!" << std::endl;
    return 0;
}
