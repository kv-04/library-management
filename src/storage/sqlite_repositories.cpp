#include "storage/sqlite_repositories.hpp"

#include <utility>

namespace library {

namespace {

// A row that was saved earlier should always be valid. If it is not, the data file was
// edited or damaged outside this program, which is a storage problem, not user input.
tl::unexpected<Error> damaged_row(const Error& cause) {
    return make_error(ErrorCode::StorageFailure, "damaged data in database: " + cause.message);
}

Result<Book> read_book(const sqlite::Statement& row, int first_column) {
    auto book = Book::create(row.column_int(first_column), row.column_text(first_column + 1),
                             row.column_text(first_column + 2));
    if (!book) {
        return damaged_row(book.error());
    }
    return book;
}

Result<Student> read_student(const sqlite::Statement& row) {
    auto student = Student::create(row.column_int(0), row.column_text(1), row.column_text(2));
    if (!student) {
        return damaged_row(student.error());
    }
    return student;
}

// For UPDATE/DELETE: the command ran, but did it touch anything?
Result<void> require_a_row_changed(const sqlite::Database& db, const std::string& what) {
    if (db.changes() == 0) {
        return make_error(ErrorCode::NotFound, what);
    }
    return {};
}

} // namespace

// -------------------------------------------------------------------------------- books

Result<void> SqliteBookRepository::add(const Book& book) {
    return db_.run("INSERT INTO books (book_no, name, purpose) VALUES (?, ?, ?)", book.book_no(),
                   book.name(), book.purpose());
}

Result<std::optional<Book>> SqliteBookRepository::find(int book_no) {
    auto statement =
        db_.query("SELECT book_no, name, purpose FROM books WHERE book_no = ?", book_no);
    if (!statement) {
        return tl::make_unexpected(statement.error());
    }
    auto has_row = statement->step();
    if (!has_row) {
        return tl::make_unexpected(has_row.error());
    }
    if (!*has_row) {
        return std::optional<Book>{};
    }
    auto book = read_book(*statement, 0);
    if (!book) {
        return tl::make_unexpected(book.error());
    }
    return std::optional<Book>(std::move(*book));
}

Result<void> SqliteBookRepository::update(const Book& book) {
    if (auto result = db_.run("UPDATE books SET name = ?, purpose = ? WHERE book_no = ?",
                              book.name(), book.purpose(), book.book_no());
        !result) {
        return result;
    }
    return require_a_row_changed(db_, "no book with number " + std::to_string(book.book_no()));
}

Result<void> SqliteBookRepository::remove(int book_no) {
    if (auto result = db_.run("DELETE FROM books WHERE book_no = ?", book_no); !result) {
        return result;
    }
    return require_a_row_changed(db_, "no book with number " + std::to_string(book_no));
}

Result<std::vector<Book>> SqliteBookRepository::list() {
    auto statement = db_.query("SELECT book_no, name, purpose FROM books ORDER BY book_no");
    if (!statement) {
        return tl::make_unexpected(statement.error());
    }
    std::vector<Book> books;
    while (true) {
        auto has_row = statement->step();
        if (!has_row) {
            return tl::make_unexpected(has_row.error());
        }
        if (!*has_row) {
            return books;
        }
        auto book = read_book(*statement, 0);
        if (!book) {
            return tl::make_unexpected(book.error());
        }
        books.push_back(std::move(*book));
    }
}

// ----------------------------------------------------------------------------- students

Result<void> SqliteStudentRepository::add(const Student& student) {
    return db_.run("INSERT INTO students (roll_no, name, school) VALUES (?, ?, ?)",
                   student.roll_no(), student.name(), student.school());
}

Result<std::optional<Student>> SqliteStudentRepository::find(int roll_no) {
    auto statement =
        db_.query("SELECT roll_no, name, school FROM students WHERE roll_no = ?", roll_no);
    if (!statement) {
        return tl::make_unexpected(statement.error());
    }
    auto has_row = statement->step();
    if (!has_row) {
        return tl::make_unexpected(has_row.error());
    }
    if (!*has_row) {
        return std::optional<Student>{};
    }
    auto student = read_student(*statement);
    if (!student) {
        return tl::make_unexpected(student.error());
    }
    return std::optional<Student>(std::move(*student));
}

Result<void> SqliteStudentRepository::update(const Student& student) {
    if (auto result = db_.run("UPDATE students SET name = ?, school = ? WHERE roll_no = ?",
                              student.name(), student.school(), student.roll_no());
        !result) {
        return result;
    }
    return require_a_row_changed(db_, "no student with roll number " +
                                          std::to_string(student.roll_no()));
}

Result<void> SqliteStudentRepository::remove(int roll_no) {
    if (auto result = db_.run("DELETE FROM students WHERE roll_no = ?", roll_no); !result) {
        return result;
    }
    return require_a_row_changed(db_, "no student with roll number " + std::to_string(roll_no));
}

Result<std::vector<Student>> SqliteStudentRepository::list() {
    auto statement = db_.query("SELECT roll_no, name, school FROM students ORDER BY roll_no");
    if (!statement) {
        return tl::make_unexpected(statement.error());
    }
    std::vector<Student> students;
    while (true) {
        auto has_row = statement->step();
        if (!has_row) {
            return tl::make_unexpected(has_row.error());
        }
        if (!*has_row) {
            return students;
        }
        auto student = read_student(*statement);
        if (!student) {
            return tl::make_unexpected(student.error());
        }
        students.push_back(std::move(*student));
    }
}

// ------------------------------------------------------------------------------- issues

Result<void> SqliteIssueRepository::add_issue(int book_no, int roll_no,
                                              const std::string& issued_on) {
    return db_.run("INSERT INTO issues (book_no, roll_no, issued_on) VALUES (?, ?, ?)", book_no,
                   roll_no, issued_on);
}

Result<void> SqliteIssueRepository::mark_returned(int book_no, const std::string& returned_on) {
    if (auto result =
            db_.run("UPDATE issues SET returned_on = ? WHERE book_no = ? AND returned_on IS NULL",
                    returned_on, book_no);
        !result) {
        return result;
    }
    return require_a_row_changed(db_, "book " + std::to_string(book_no) + " is not issued");
}

Result<std::vector<IssueRecord>> SqliteIssueRepository::list() {
    auto statement = db_.query(R"sql(
        SELECT i.id, i.book_no, i.roll_no, s.name, i.issued_on, i.returned_on
        FROM issues i JOIN students s ON s.roll_no = i.roll_no
        ORDER BY i.id DESC)sql");
    if (!statement) {
        return tl::make_unexpected(statement.error());
    }
    std::vector<IssueRecord> records;
    while (true) {
        auto has_row = statement->step();
        if (!has_row) {
            return tl::make_unexpected(has_row.error());
        }
        if (!*has_row) {
            return records;
        }
        IssueRecord record{statement->column_int(0),  statement->column_int(1),
                           statement->column_int(2),  statement->column_text(3),
                           statement->column_text(4), std::nullopt};
        if (!statement->column_is_null(5)) {
            record.returned_on = statement->column_text(5);
        }
        records.push_back(std::move(record));
    }
}

// -------------------------------------------------------------------------------- users

Result<void> SqliteUserRepository::add(const std::string& username,
                                       const std::string& password_hash) {
    return db_.run("INSERT INTO users (username, password_hash) VALUES (?, ?)", username,
                   password_hash);
}

Result<std::optional<std::string>>
SqliteUserRepository::find_password_hash(const std::string& username) {
    auto statement = db_.query("SELECT password_hash FROM users WHERE username = ?", username);
    if (!statement) {
        return tl::make_unexpected(statement.error());
    }
    auto has_row = statement->step();
    if (!has_row) {
        return tl::make_unexpected(has_row.error());
    }
    if (!*has_row) {
        return std::optional<std::string>{};
    }
    return std::optional<std::string>(statement->column_text(0));
}

Result<int> SqliteUserRepository::count() {
    auto statement = db_.query("SELECT COUNT(*) FROM users");
    if (!statement) {
        return tl::make_unexpected(statement.error());
    }
    auto has_row = statement->step();
    if (!has_row) {
        return tl::make_unexpected(has_row.error());
    }
    return statement->column_int(0);
}

// ------------------------------------------------------------------------------- backup

Result<void> SqliteBackupStore::save_copy(const std::string& path) {
    return db_.backup_to(path);
}

} // namespace library
