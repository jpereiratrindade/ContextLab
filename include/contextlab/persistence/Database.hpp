#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include "contextlab/core/Result.hpp"

namespace contextlab::persistence {

class Statement {
public:
    explicit Statement(sqlite3_stmt* stmt);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    void bindInt64(int index, int64_t val);
    void bindDouble(int index, double val);
    void bindText(int index, std::string_view val);
    void bindNull(int index);

    [[nodiscard]] bool step();
    void reset();

    [[nodiscard]] int64_t getInt64(int col) const;
    [[nodiscard]] double getDouble(int col) const;
    [[nodiscard]] std::string getText(int col) const;
    [[nodiscard]] bool isNull(int col) const;

private:
    sqlite3_stmt* stmt_{nullptr};
};

class Database {
public:
    explicit Database(const std::filesystem::path& db_path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    [[nodiscard]] core::Result<void> open();
    void close();

    [[nodiscard]] core::Result<void> execute(std::string_view sql);
    [[nodiscard]] core::Result<Statement> prepare(std::string_view sql);

    [[nodiscard]] int64_t lastInsertRowId() const;
    [[nodiscard]] int changes() const;

    [[nodiscard]] core::Result<void> beginTransaction();
    [[nodiscard]] core::Result<void> commit();
    [[nodiscard]] core::Result<void> rollback();

    [[nodiscard]] bool isOpen() const noexcept { return db_ != nullptr; }

private:
    std::filesystem::path db_path_;
    sqlite3* db_{nullptr};
    mutable std::mutex mutex_;
};

} // namespace contextlab::persistence
