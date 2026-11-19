#pragma once

#include "domain/error.hpp"

#include <memory>
#include <string>
#include <string_view>

// SQLite's C types, declared here so this header does not need to include sqlite3.h.
struct sqlite3;
struct sqlite3_stmt;

namespace library::sqlite {

// Tells std::unique_ptr how to release a SQLite handle. unique_ptr calls these automatically
// when it goes out of scope, so nobody can forget (or double-call) the cleanup.
struct DatabaseCloser {
    void operator()(sqlite3* db) const noexcept;
};
struct StatementFinalizer {
    void operator()(sqlite3_stmt* statement) const noexcept;
};

// One prepared SQL command. Create it with Database::prepare().
//
// Numbering follows SQLite: parameters (the ? marks in the SQL) count from 1, result
// columns count from 0.
class Statement {
  public:
    [[nodiscard]] Result<void> bind(int index, int value);
    [[nodiscard]] Result<void> bind(int index, std::string_view value);
    [[nodiscard]] Result<void> bind_null(int index);

    // Binds every argument to parameters 1, 2, 3, ... in order. Stops at the first failure.
    template <typename... Args> [[nodiscard]] Result<void> bind_all(const Args&... args) {
        Result<void> result{};
        [[maybe_unused]] int index = 0;
        [[maybe_unused]] const auto bind_one = [&](const auto& value) {
            if (result) {
                result = bind(++index, value);
            }
        };
        (bind_one(args), ...);
        return result;
    }

    // Runs the command until it produces a row or finishes.
    // true = a row is ready to read, false = done.
    [[nodiscard]] Result<bool> step();

    // Forget the current progress and bound values so the statement can be used again.
    [[nodiscard]] Result<void> reset();

    [[nodiscard]] int column_int(int column) const;
    [[nodiscard]] std::string column_text(int column) const;
    [[nodiscard]] bool column_is_null(int column) const;

  private:
    friend class Database;
    explicit Statement(std::unique_ptr<sqlite3_stmt, StatementFinalizer> statement);

    std::unique_ptr<sqlite3_stmt, StatementFinalizer> statement_;
};

// An open database file (or ":memory:" for a temporary one that lives only in RAM).
// Movable, not copyable: there is exactly one owner of the underlying connection.
class Database {
  public:
    // Opens the file, creating it if it does not exist, and turns on foreign-key checking.
    [[nodiscard]] static Result<Database> open(const std::string& path);

    // Runs SQL that returns no rows (CREATE TABLE, INSERT, PRAGMA, ...).
    [[nodiscard]] Result<void> execute(const std::string& sql);

    [[nodiscard]] Result<Statement> prepare(const std::string& sql);

    // prepare() + bind_all() in one call: query("... WHERE id = ?", 7).
    template <typename... Args>
    [[nodiscard]] Result<Statement> query(const std::string& sql, const Args&... args);

    // Runs one command that returns no rows, with bound arguments:
    // run("INSERT INTO t (a, b) VALUES (?, ?)", 1, "x").
    template <typename... Args>
    [[nodiscard]] Result<void> run(const std::string& sql, const Args&... args);

    // Writes a complete, consistent copy of the database to a NEW file.
    // Fails if the file already exists.
    [[nodiscard]] Result<void> backup_to(const std::string& path);

    // Rows changed by the most recent INSERT, UPDATE or DELETE.
    [[nodiscard]] int changes() const noexcept;

  private:
    explicit Database(std::unique_ptr<sqlite3, DatabaseCloser> db);

    std::unique_ptr<sqlite3, DatabaseCloser> db_;
};

template <typename... Args>
Result<Statement> Database::query(const std::string& sql, const Args&... args) {
    auto statement = prepare(sql);
    if (!statement) {
        return tl::make_unexpected(statement.error());
    }
    if (auto bound = statement->bind_all(args...); !bound) {
        return tl::make_unexpected(bound.error());
    }
    return statement;
}

template <typename... Args>
Result<void> Database::run(const std::string& sql, const Args&... args) {
    auto statement = query(sql, args...);
    if (!statement) {
        return tl::make_unexpected(statement.error());
    }
    if (auto stepped = statement->step(); !stepped) {
        return tl::make_unexpected(stepped.error());
    }
    return {};
}

} // namespace library::sqlite
