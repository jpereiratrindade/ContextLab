#include "contextlab/persistence/Database.hpp"
#include <iostream>

namespace contextlab::persistence {

Statement::Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}

Statement::~Statement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

Statement::Statement(Statement&& other) noexcept : stmt_(other.stmt_) {
    other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (stmt_) sqlite3_finalize(stmt_);
        stmt_ = other.stmt_;
        other.stmt_ = nullptr;
    }
    return *this;
}

void Statement::bindInt64(int index, int64_t val) {
    sqlite3_bind_int64(stmt_, index, val);
}

void Statement::bindDouble(int index, double val) {
    sqlite3_bind_double(stmt_, index, val);
}

void Statement::bindText(int index, std::string_view val) {
    sqlite3_bind_text(stmt_, index, val.data(), static_cast<int>(val.size()), SQLITE_TRANSIENT);
}

void Statement::bindNull(int index) {
    sqlite3_bind_null(stmt_, index);
}

bool Statement::step() {
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) return true;
    if (rc == SQLITE_DONE) return false;
    return false;
}

void Statement::reset() {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

int64_t Statement::getInt64(int col) const {
    return sqlite3_column_int64(stmt_, col);
}

double Statement::getDouble(int col) const {
    return sqlite3_column_double(stmt_, col);
}

std::string Statement::getText(int col) const {
    const auto* txt = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, col));
    return txt ? std::string(txt) : "";
}

bool Statement::isNull(int col) const {
    return sqlite3_column_type(stmt_, col) == SQLITE_NULL;
}

Database::Database(const std::filesystem::path& db_path) : db_path_(db_path) {}

Database::~Database() {
    close();
}

core::Result<void> Database::open() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) return core::makeOk();

    if (db_path_.has_parent_path()) {
        std::filesystem::create_directories(db_path_.parent_path());
    }

    int rc = sqlite3_open(db_path_.string().c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err_msg = sqlite3_errmsg(db_);
        close();
        return core::makeError(core::ErrorCode::DATABASE_ERROR, "Failed to open SQLite database: " + err_msg);
    }

    // Enable WAL mode and foreign keys for performance and data safety
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

    return core::makeOk();
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

core::Result<void> Database::execute(std::string_view sql) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Database is not open");

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.data(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string err = err_msg ? err_msg : "Unknown SQLite error";
        sqlite3_free(err_msg);
        return core::makeError(core::ErrorCode::DATABASE_ERROR, "SQL execution error: " + err);
    }

    return core::makeOk();
}

core::Result<Statement> Database::prepare(std::string_view sql) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return core::makeError(core::ErrorCode::DATABASE_ERROR, "Database is not open");

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.data(), static_cast<int>(sql.size()), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        return core::makeError(core::ErrorCode::DATABASE_ERROR, "SQL prepare failed: " + err);
    }

    return core::makeOk(Statement(stmt));
}

int64_t Database::lastInsertRowId() const {
    return db_ ? sqlite3_last_insert_rowid(db_) : 0;
}

int Database::changes() const {
    return db_ ? sqlite3_changes(db_) : 0;
}

core::Result<void> Database::beginTransaction() {
    return execute("BEGIN TRANSACTION;");
}

core::Result<void> Database::commit() {
    return execute("COMMIT;");
}

core::Result<void> Database::rollback() {
    return execute("ROLLBACK;");
}

} // namespace contextlab::persistence
