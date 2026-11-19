#pragma once

#include "domain/book.hpp"
#include "domain/error.hpp"
#include "domain/student.hpp"

#include <optional>
#include <string>
#include <vector>

namespace library {

// These are the "sockets" the rest of the program plugs into. They say WHAT can be saved and
// loaded, not HOW. The SQLite versions live in storage/. Anything that can fail returns a
// Result; "not found" on a lookup is a normal answer, so find() returns an optional inside it.

class BookRepository {
  public:
    virtual ~BookRepository() = default;

    [[nodiscard]] virtual Result<void> add(const Book& book) = 0; // AlreadyExists
    [[nodiscard]] virtual Result<std::optional<Book>> find(int book_no) = 0;
    [[nodiscard]] virtual Result<void> update(const Book& book) = 0; // NotFound
    [[nodiscard]] virtual Result<void> remove(int book_no) = 0;      // NotFound, Conflict
    [[nodiscard]] virtual Result<std::vector<Book>> list() = 0;      // by book number
};

class StudentRepository {
  public:
    virtual ~StudentRepository() = default;

    [[nodiscard]] virtual Result<void> add(const Student& student) = 0; // AlreadyExists
    [[nodiscard]] virtual Result<std::optional<Student>> find(int roll_no) = 0;
    [[nodiscard]] virtual Result<void> update(const Student& student) = 0; // NotFound
    [[nodiscard]] virtual Result<void> remove(int roll_no) = 0;            // NotFound, Conflict
    [[nodiscard]] virtual Result<std::vector<Student>> list() = 0;         // by roll number
};

// One line of the loan history.
struct IssueRecord {
    int id;
    int book_no;
    int roll_no;
    std::string student_name;
    std::string issued_on;                  // "YYYY-MM-DD"
    std::optional<std::string> returned_on; // empty while the book is still out
};

class IssueRepository {
  public:
    virtual ~IssueRepository() = default;

    // AlreadyExists: the book is already out. Conflict: no such book or student.
    [[nodiscard]] virtual Result<void> add_issue(int book_no, int roll_no,
                                                 const std::string& issued_on) = 0;
    // NotFound: that book is not currently issued.
    [[nodiscard]] virtual Result<void> mark_returned(int book_no,
                                                     const std::string& returned_on) = 0;
    [[nodiscard]] virtual Result<std::vector<IssueRecord>> list() = 0; // newest first
};

class UserRepository {
  public:
    virtual ~UserRepository() = default;

    [[nodiscard]] virtual Result<void> add(const std::string& username,
                                           const std::string& password_hash) = 0; // AlreadyExists
    [[nodiscard]] virtual Result<std::optional<std::string>>
    find_password_hash(const std::string& username) = 0;
    [[nodiscard]] virtual Result<int> count() = 0;
};

class BackupStore {
  public:
    virtual ~BackupStore() = default;

    // Saves a full copy of all data to a new file.
    [[nodiscard]] virtual Result<void> save_copy(const std::string& path) = 0;
};

} // namespace library
