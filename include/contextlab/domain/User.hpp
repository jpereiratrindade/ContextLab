#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace contextlab::domain {

struct User {
    std::string email;
    std::string name;
    std::string role{"researcher"};  // researcher, manager, admin, guest
    std::string unit{"Embrapa"};     // e.g. Embrapa Pecuária Sul, Embrapa Agricultura Digital
    std::string created_at;
    std::string last_login_at;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"email", email},
            {"name", name},
            {"role", role},
            {"unit", unit},
            {"created_at", created_at},
            {"last_login_at", last_login_at}
        };
    }
};

struct AuthOtp {
    std::string email;
    std::string otp_code;
    std::string expires_at;
    std::string created_at;
};

struct AuthSession {
    std::string token;
    std::string email;
    std::string created_at;
    std::string expires_at;
};

struct TopicGraphNode {
    std::string id;
    std::string label;
    std::string category; // e.g. "Clima & Carbono", "Sistemas Produtivos", "Biotecnologia", "Agro Digital"
    int count{1};
    double weight{1.0};
    bool is_user_interest{false};

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"id", id},
            {"label", label},
            {"category", category},
            {"count", count},
            {"weight", weight},
            {"is_user_interest", is_user_interest}
        };
    }
};

struct TopicGraphEdge {
    std::string source;
    std::string target;
    int weight{1};
    std::string relation_type{"co-occurs"};

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"source", source},
            {"target", target},
            {"weight", weight},
            {"relation_type", relation_type}
        };
    }
};

struct TopicGraphData {
    std::vector<TopicGraphNode> nodes;
    std::vector<TopicGraphEdge> edges;
    int total_documents{0};
    int total_topics{0};

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json n_arr = nlohmann::json::array();
        for (const auto& n : nodes) n_arr.push_back(n.toJson());

        nlohmann::json e_arr = nlohmann::json::array();
        for (const auto& e : edges) e_arr.push_back(e.toJson());

        return {
            {"nodes", n_arr},
            {"edges", e_arr},
            {"total_documents", total_documents},
            {"total_topics", total_topics}
        };
    }
};

} // namespace contextlab::domain
