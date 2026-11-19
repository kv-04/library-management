#pragma once

#include "service/auth_service.hpp"
#include "service/library_service.hpp"
#include "ui/console.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace library {

// A file name for a new backup, such as "library-backup-20261113-193000.db".
[[nodiscard]] std::string default_backup_name();

// The menus. Only asks questions and shows answers; every rule lives in the services.
class ConsoleApp {
  public:
    using NameProvider = std::function<std::string()>;

    ConsoleApp(Console& console, LibraryService& library, AuthService& auth,
               NameProvider backup_name = &default_backup_name);

    // Runs until the user exits. Returns 0 after a normal session, 1 if login failed or the
    // input ended before login, 2 if the database could not be used.
    int run();

  private:
    // Login
    bool create_first_account();
    bool log_in();

    // Menus
    void main_menu();
    void book_menu();
    void student_menu();
    void issue_menu();

    // Books
    void add_book();
    void list_books();
    void find_book();
    void update_book();
    void delete_book();

    // Students
    void add_student();
    void list_students();
    void find_student();
    void update_student();
    void delete_student();

    // Loans and backup
    void issue_book();
    void return_book();
    void show_issue_history();
    void back_up();

    // Helpers
    std::optional<int> choose(const std::string& title, const std::vector<std::string>& options,
                              const std::string& zero_label);
    void report(const Result<void>& result, const std::string& success_message);
    void print_error(const Error& error);

    Console& console_;
    LibraryService& library_;
    AuthService& auth_;
    NameProvider backup_name_;
};

} // namespace library
