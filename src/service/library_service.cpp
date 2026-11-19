#include "service/library_service.hpp"

#include <ctime>
#include <utility>

namespace library {

namespace {

// If `result` failed with the error code `when`, swap in a friendlier message.
Result<void> with_message(Result<void> result, ErrorCode when, const std::string& message) {
    if (!result && result.error().code == when) {
        return make_error(when, message);
    }
    return result;
}

std::string book_label(int book_no) {
    return "book number " + std::to_string(book_no);
}

std::string student_label(int roll_no) {
    return "student with roll number " + std::to_string(roll_no);
}

} // namespace

std::string todays_date() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &local);
    return buffer;
}

LibraryService::LibraryService(BookRepository& books, StudentRepository& students,
                               IssueRepository& issues, BackupStore& backups, TodayProvider today)
    : books_(books), students_(students), issues_(issues), backups_(backups),
      today_(std::move(today)) {}

// ------------------------------------------------------------------------------- books

Result<void> LibraryService::add_book(int book_no, std::string name, std::string purpose) {
    auto book = Book::create(book_no, std::move(name), std::move(purpose));
    if (!book) {
        return tl::make_unexpected(book.error());
    }
    return with_message(books_.add(*book), ErrorCode::AlreadyExists,
                        book_label(book_no) + " already exists");
}

Result<std::optional<Book>> LibraryService::find_book(int book_no) {
    return books_.find(book_no);
}

Result<void> LibraryService::update_book(int book_no, std::string name, std::string purpose) {
    auto book = Book::create(book_no, std::move(name), std::move(purpose));
    if (!book) {
        return tl::make_unexpected(book.error());
    }
    return books_.update(*book);
}

Result<void> LibraryService::remove_book(int book_no) {
    return with_message(books_.remove(book_no), ErrorCode::Conflict,
                        book_label(book_no) + " has loan history and cannot be deleted");
}

Result<std::vector<Book>> LibraryService::list_books() {
    return books_.list();
}

// ----------------------------------------------------------------------------- students

Result<void> LibraryService::add_student(int roll_no, std::string name, std::string school) {
    auto student = Student::create(roll_no, std::move(name), std::move(school));
    if (!student) {
        return tl::make_unexpected(student.error());
    }
    return with_message(students_.add(*student), ErrorCode::AlreadyExists,
                        student_label(roll_no) + " already exists");
}

Result<std::optional<Student>> LibraryService::find_student(int roll_no) {
    return students_.find(roll_no);
}

Result<void> LibraryService::update_student(int roll_no, std::string name, std::string school) {
    auto student = Student::create(roll_no, std::move(name), std::move(school));
    if (!student) {
        return tl::make_unexpected(student.error());
    }
    return students_.update(*student);
}

Result<void> LibraryService::remove_student(int roll_no) {
    return with_message(students_.remove(roll_no), ErrorCode::Conflict,
                        student_label(roll_no) + " has loan history and cannot be deleted");
}

Result<std::vector<Student>> LibraryService::list_students() {
    return students_.list();
}

// -------------------------------------------------------------------------------- loans

Result<void> LibraryService::issue_book(int book_no, int roll_no) {
    auto book = books_.find(book_no);
    if (!book) {
        return tl::make_unexpected(book.error());
    }
    if (!book->has_value()) {
        return make_error(ErrorCode::NotFound, book_label(book_no) + " does not exist");
    }

    auto student = students_.find(roll_no);
    if (!student) {
        return tl::make_unexpected(student.error());
    }
    if (!student->has_value()) {
        return make_error(ErrorCode::NotFound, student_label(roll_no) + " does not exist");
    }

    return with_message(issues_.add_issue(book_no, roll_no, today_()), ErrorCode::AlreadyExists,
                        book_label(book_no) + " is already issued");
}

Result<void> LibraryService::return_book(int book_no) {
    return with_message(issues_.mark_returned(book_no, today_()), ErrorCode::NotFound,
                        book_label(book_no) + " is not currently issued");
}

Result<std::vector<IssueRecord>> LibraryService::list_issues() {
    return issues_.list();
}

Result<void> LibraryService::backup(const std::string& path) {
    return backups_.save_copy(path);
}

} // namespace library
