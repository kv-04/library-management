#pragma once

#include "domain/repositories.hpp"
#include "storage/sqlite.hpp"

namespace library {

// The SQLite versions of the repository interfaces. Each one borrows the Database
// (it must outlive them) and runs its own SQL.

class SqliteBookRepository final : public BookRepository {
  public:
    explicit SqliteBookRepository(sqlite::Database& db) : db_(db) {}

    Result<void> add(const Book& book) override;
    Result<std::optional<Book>> find(int book_no) override;
    Result<void> update(const Book& book) override;
    Result<void> remove(int book_no) override;
    Result<std::vector<Book>> list() override;

  private:
    sqlite::Database& db_;
};

class SqliteStudentRepository final : public StudentRepository {
  public:
    explicit SqliteStudentRepository(sqlite::Database& db) : db_(db) {}

    Result<void> add(const Student& student) override;
    Result<std::optional<Student>> find(int roll_no) override;
    Result<void> update(const Student& student) override;
    Result<void> remove(int roll_no) override;
    Result<std::vector<Student>> list() override;

  private:
    sqlite::Database& db_;
};

class SqliteIssueRepository final : public IssueRepository {
  public:
    explicit SqliteIssueRepository(sqlite::Database& db) : db_(db) {}

    Result<void> add_issue(int book_no, int roll_no, const std::string& issued_on) override;
    Result<void> mark_returned(int book_no, const std::string& returned_on) override;
    Result<std::vector<IssueRecord>> list() override;

  private:
    sqlite::Database& db_;
};

class SqliteUserRepository final : public UserRepository {
  public:
    explicit SqliteUserRepository(sqlite::Database& db) : db_(db) {}

    Result<void> add(const std::string& username, const std::string& password_hash) override;
    Result<std::optional<std::string>> find_password_hash(const std::string& username) override;
    Result<int> count() override;

  private:
    sqlite::Database& db_;
};

class SqliteBackupStore final : public BackupStore {
  public:
    explicit SqliteBackupStore(sqlite::Database& db) : db_(db) {}

    Result<void> save_copy(const std::string& path) override;

  private:
    sqlite::Database& db_;
};

} // namespace library
