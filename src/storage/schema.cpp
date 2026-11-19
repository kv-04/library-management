#include "storage/schema.hpp"

namespace library {

Result<void> create_schema(sqlite::Database& db) {
    return db.execute(R"sql(
        CREATE TABLE IF NOT EXISTS books (
            book_no INTEGER PRIMARY KEY,
            name    TEXT NOT NULL,
            purpose TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS students (
            roll_no INTEGER PRIMARY KEY,
            name    TEXT NOT NULL,
            school  TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS issues (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            book_no     INTEGER NOT NULL REFERENCES books(book_no),
            roll_no     INTEGER NOT NULL REFERENCES students(roll_no),
            issued_on   TEXT NOT NULL,
            returned_on TEXT
        );

        -- A book can be out only once at a time: at most one row per book with no return date.
        CREATE UNIQUE INDEX IF NOT EXISTS one_open_issue_per_book
            ON issues(book_no) WHERE returned_on IS NULL;

        CREATE TABLE IF NOT EXISTS users (
            username      TEXT PRIMARY KEY,
            password_hash TEXT NOT NULL
        );
    )sql");
}

} // namespace library
