#pragma once

#include "domain/repositories.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace library {

// Today's date as "YYYY-MM-DD" in local time.
[[nodiscard]] std::string todays_date();

// The library's rules. The menus call this; this calls the repositories. It turns raw storage
// errors into messages a person can understand ("book number 7 is already issued").
class LibraryService {
  public:
    // `today` is a function so tests can pretend it is any date they like.
    using TodayProvider = std::function<std::string()>;

    LibraryService(BookRepository& books, StudentRepository& students, IssueRepository& issues,
                   BackupStore& backups, TodayProvider today = &todays_date);

    // Books
    [[nodiscard]] Result<void> add_book(int book_no, std::string name, std::string purpose);
    [[nodiscard]] Result<std::optional<Book>> find_book(int book_no);
    [[nodiscard]] Result<void> update_book(int book_no, std::string name, std::string purpose);
    [[nodiscard]] Result<void> remove_book(int book_no);
    [[nodiscard]] Result<std::vector<Book>> list_books();

    // Students
    [[nodiscard]] Result<void> add_student(int roll_no, std::string name, std::string school);
    [[nodiscard]] Result<std::optional<Student>> find_student(int roll_no);
    [[nodiscard]] Result<void> update_student(int roll_no, std::string name, std::string school);
    [[nodiscard]] Result<void> remove_student(int roll_no);
    [[nodiscard]] Result<std::vector<Student>> list_students();

    // Loans
    [[nodiscard]] Result<void> issue_book(int book_no, int roll_no);
    [[nodiscard]] Result<void> return_book(int book_no);
    [[nodiscard]] Result<std::vector<IssueRecord>> list_issues();

    // Saves a copy of all data to a new file.
    [[nodiscard]] Result<void> backup(const std::string& path);

  private:
    BookRepository& books_;
    StudentRepository& students_;
    IssueRepository& issues_;
    BackupStore& backups_;
    TodayProvider today_;
};

} // namespace library
