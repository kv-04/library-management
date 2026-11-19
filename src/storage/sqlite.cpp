#include "storage/sqlite.hpp"

#include <cstdint>
#include <sqlite3.h>
#include <utility>

namespace library::sqlite {

namespace {

ErrorCode classify(int result_code) {
    // Extended result codes are switched on in Database::open().
    switch (result_code) {
    case SQLITE_CONSTRAINT_PRIMARYKEY:
    case SQLITE_CONSTRAINT_UNIQUE:
        return ErrorCode::AlreadyExists;
    case SQLITE_CONSTRAINT_FOREIGNKEY:
        return ErrorCode::Conflict;
    default:
        return ErrorCode::StorageFailure;
    }
}

tl::unexpected<Error> failure(sqlite3* db, int result_code, std::string_view what) {
    std::string message(what);
    message += ": ";
    message += (db != nullptr) ? sqlite3_errmsg(db) : "out of memory";
    return make_error(classify(result_code), std::move(message));
}

// SQLite's own SQLITE_TRANSIENT macro uses a C-style cast, which our strict warnings reject.
// This is the same value: "copy the text now, I may free mine right after".
sqlite3_destructor_type copy_text() {
    return reinterpret_cast<sqlite3_destructor_type>(static_cast<std::intptr_t>(-1));
}

} // namespace

void DatabaseCloser::operator()(sqlite3* db) const noexcept {
    // _v2 postpones the real close until every statement is finalized, so destruction
    // order between a Database and its Statements can never leak or crash.
    sqlite3_close_v2(db);
}

void StatementFinalizer::operator()(sqlite3_stmt* statement) const noexcept {
    sqlite3_finalize(statement);
}

// ---------------------------------------------------------------- Statement

Statement::Statement(std::unique_ptr<sqlite3_stmt, StatementFinalizer> statement)
    : statement_(std::move(statement)) {}

Result<void> Statement::bind(int index, int value) {
    const int rc = sqlite3_bind_int(statement_.get(), index, value);
    if (rc != SQLITE_OK) {
        return failure(sqlite3_db_handle(statement_.get()), rc, "bind integer");
    }
    return {};
}

Result<void> Statement::bind(int index, std::string_view value) {
    // An empty string_view may hold a null pointer, which SQLite would store as NULL.
    const char* text = value.empty() ? "" : value.data();
    const int rc =
        sqlite3_bind_text64(statement_.get(), index, text, value.size(), copy_text(), SQLITE_UTF8);
    if (rc != SQLITE_OK) {
        return failure(sqlite3_db_handle(statement_.get()), rc, "bind text");
    }
    return {};
}

Result<void> Statement::bind_null(int index) {
    const int rc = sqlite3_bind_null(statement_.get(), index);
    if (rc != SQLITE_OK) {
        return failure(sqlite3_db_handle(statement_.get()), rc, "bind null");
    }
    return {};
}

Result<bool> Statement::step() {
    const int rc = sqlite3_step(statement_.get());
    if (rc == SQLITE_ROW) {
        return true;
    }
    if (rc == SQLITE_DONE) {
        return false;
    }
    return failure(sqlite3_db_handle(statement_.get()), rc, "run statement");
}

Result<void> Statement::reset() {
    // reset reports the error of the previous step() again; that was already returned
    // to the caller, so only a failure to reset itself matters here.
    sqlite3_reset(statement_.get());
    const int rc = sqlite3_clear_bindings(statement_.get());
    if (rc != SQLITE_OK) {
        return failure(sqlite3_db_handle(statement_.get()), rc, "reset statement");
    }
    return {};
}

int Statement::column_int(int column) const {
    return sqlite3_column_int(statement_.get(), column);
}

std::string Statement::column_text(int column) const {
    const unsigned char* text = sqlite3_column_text(statement_.get(), column);
    if (text == nullptr) {
        return {};
    }
    const auto size = static_cast<std::size_t>(sqlite3_column_bytes(statement_.get(), column));
    return std::string(reinterpret_cast<const char*>(text), size);
}

bool Statement::column_is_null(int column) const {
    return sqlite3_column_type(statement_.get(), column) == SQLITE_NULL;
}

// ----------------------------------------------------------------- Database

Database::Database(std::unique_ptr<sqlite3, DatabaseCloser> db) : db_(std::move(db)) {}

Result<Database> Database::open(const std::string& path) {
    sqlite3* raw = nullptr;
    const int rc =
        sqlite3_open_v2(path.c_str(), &raw, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    // SQLite hands back a handle even when opening fails, so take ownership immediately:
    // the unique_ptr then closes it on every path out of this function.
    std::unique_ptr<sqlite3, DatabaseCloser> handle(raw);
    if (rc != SQLITE_OK) {
        return failure(raw, rc, "open database '" + path + "'");
    }

    sqlite3_extended_result_codes(raw, 1);

    Database database(std::move(handle));
    // SQLite ignores foreign keys unless asked, for historical reasons.
    if (auto pragma = database.execute("PRAGMA foreign_keys = ON"); !pragma) {
        return tl::make_unexpected(pragma.error());
    }
    return database;
}

Result<void> Database::execute(const std::string& sql) {
    const int rc = sqlite3_exec(db_.get(), sql.c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        return failure(db_.get(), rc, "execute SQL");
    }
    return {};
}

Result<Statement> Database::prepare(const std::string& sql) {
    sqlite3_stmt* raw = nullptr;
    const int rc = sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &raw, nullptr);
    std::unique_ptr<sqlite3_stmt, StatementFinalizer> statement(raw);
    if (rc != SQLITE_OK) {
        return failure(db_.get(), rc, "prepare SQL");
    }
    return Statement(std::move(statement));
}

Result<void> Database::backup_to(const std::string& path) {
    return run("VACUUM INTO ?", path);
}

int Database::changes() const noexcept {
    return sqlite3_changes(db_.get());
}

} // namespace library::sqlite
