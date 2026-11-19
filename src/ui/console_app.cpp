#include "ui/console_app.hpp"

#include "domain/text.hpp"

#include <ctime>
#include <iomanip>
#include <utility>

namespace library {

namespace {

constexpr int kMaxLoginAttempts = 3;

} // namespace

std::string default_backup_name() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    char buffer[40];
    std::strftime(buffer, sizeof(buffer), "library-backup-%Y%m%d-%H%M%S.db", &local);
    return buffer;
}

ConsoleApp::ConsoleApp(Console& console, LibraryService& library, AuthService& auth,
                       NameProvider backup_name)
    : console_(console), library_(library), auth_(auth), backup_name_(std::move(backup_name)) {}

// ------------------------------------------------------------------------------- start

int ConsoleApp::run() {
    console_.println();
    console_.println("=======================================");
    console_.println("     LIBRARY MANAGEMENT SYSTEM");
    console_.println("=======================================");

    const auto has_users = auth_.has_users();
    if (!has_users) {
        print_error(has_users.error());
        return 2;
    }
    const bool signed_in = *has_users ? log_in() : create_first_account();
    if (!signed_in) {
        return 1;
    }

    console_.println("Welcome!");
    main_menu();
    console_.println("Goodbye.");
    return 0;
}

bool ConsoleApp::create_first_account() {
    console_.println("First run: let's create the administrator account.");
    while (true) {
        const auto username = console_.read_line("Choose a username: ");
        if (!username) {
            return false;
        }
        const auto password = console_.read_password("Choose a password (8+ characters): ");
        if (!password) {
            return false;
        }
        const auto repeated = console_.read_password("Repeat the password: ");
        if (!repeated) {
            return false;
        }
        if (*password != *repeated) {
            console_.println("The passwords do not match. Try again.");
            continue;
        }
        const auto created = auth_.register_user(*username, *password);
        if (created) {
            console_.println("Account created.");
            return true;
        }
        print_error(created.error());
    }
}

bool ConsoleApp::log_in() {
    for (int attempt = 1; attempt <= kMaxLoginAttempts; ++attempt) {
        const auto username = console_.read_line("Username: ");
        if (!username) {
            return false;
        }
        const auto password = console_.read_password("Password: ");
        if (!password) {
            return false;
        }
        const auto result = auth_.log_in(*username, *password);
        if (result) {
            return true;
        }
        print_error(result.error());
    }
    console_.println("Too many failed attempts.");
    return false;
}

// ------------------------------------------------------------------------------- menus

std::optional<int> ConsoleApp::choose(const std::string& title,
                                      const std::vector<std::string>& options,
                                      const std::string& zero_label) {
    console_.println();
    console_.println(title);
    console_.println(std::string(title.size(), '='));
    for (std::size_t i = 0; i < options.size(); ++i) {
        console_.println(std::to_string(i + 1) + ". " + options[i]);
    }
    console_.println("0. " + zero_label);

    while (true) {
        const auto choice = console_.read_int("Choose: ");
        if (!choice) {
            return std::nullopt;
        }
        if (*choice >= 0 && *choice <= static_cast<int>(options.size())) {
            return choice;
        }
        console_.println("Please choose a number from the list.");
    }
}

void ConsoleApp::main_menu() {
    while (true) {
        const auto choice =
            choose("MAIN MENU", {"Books", "Students", "Issue / return books", "Back up the data"},
                   "Log out and exit");
        if (!choice || *choice == 0) {
            return;
        }
        switch (*choice) {
        case 1:
            book_menu();
            break;
        case 2:
            student_menu();
            break;
        case 3:
            issue_menu();
            break;
        case 4:
            back_up();
            break;
        default:
            break;
        }
    }
}

void ConsoleApp::book_menu() {
    while (true) {
        const auto choice = choose(
            "BOOKS",
            {"Add a book", "List all books", "Find a book", "Update a book", "Delete a book"},
            "Back");
        if (!choice || *choice == 0) {
            return;
        }
        switch (*choice) {
        case 1:
            add_book();
            break;
        case 2:
            list_books();
            break;
        case 3:
            find_book();
            break;
        case 4:
            update_book();
            break;
        case 5:
            delete_book();
            break;
        default:
            break;
        }
    }
}

void ConsoleApp::student_menu() {
    while (true) {
        const auto choice = choose("STUDENTS",
                                   {"Add a student", "List all students", "Find a student",
                                    "Update a student", "Delete a student"},
                                   "Back");
        if (!choice || *choice == 0) {
            return;
        }
        switch (*choice) {
        case 1:
            add_student();
            break;
        case 2:
            list_students();
            break;
        case 3:
            find_student();
            break;
        case 4:
            update_student();
            break;
        case 5:
            delete_student();
            break;
        default:
            break;
        }
    }
}

void ConsoleApp::issue_menu() {
    while (true) {
        const auto choice =
            choose("ISSUE / RETURN", {"Issue a book", "Return a book", "Loan history"}, "Back");
        if (!choice || *choice == 0) {
            return;
        }
        switch (*choice) {
        case 1:
            issue_book();
            break;
        case 2:
            return_book();
            break;
        case 3:
            show_issue_history();
            break;
        default:
            break;
        }
    }
}

// ------------------------------------------------------------------------------- books

void ConsoleApp::add_book() {
    const auto number = console_.read_int("Book number: ");
    if (!number) {
        return;
    }
    const auto name = console_.read_line("Book name: ");
    if (!name) {
        return;
    }
    const auto purpose = console_.read_line("Purpose: ");
    if (!purpose) {
        return;
    }
    report(library_.add_book(*number, *name, *purpose), "Book added.");
}

namespace {

void print_book_table(std::ostream& out, const std::vector<Book>& books) {
    out << '\n'
        << std::left << std::setw(10) << "BOOK NO" << std::setw(26) << "NAME" << "PURPOSE\n";
    for (const Book& book : books) {
        out << std::setw(10) << book.book_no() << std::setw(26) << book.name() << book.purpose()
            << '\n';
    }
}

void print_student_table(std::ostream& out, const std::vector<Student>& students) {
    out << '\n' << std::left << std::setw(10) << "ROLL NO" << std::setw(26) << "NAME" << "SCHOOL\n";
    for (const Student& student : students) {
        out << std::setw(10) << student.roll_no() << std::setw(26) << student.name()
            << student.school() << '\n';
    }
}

// An empty answer means "keep what is there".
std::string keep_if_blank(const std::string& answer, const std::string& current) {
    return trim(answer).empty() ? current : answer;
}

} // namespace

void ConsoleApp::list_books() {
    const auto books = library_.list_books();
    if (!books) {
        print_error(books.error());
    } else if (books->empty()) {
        console_.println("No books yet.");
    } else {
        print_book_table(console_.out(), *books);
    }
}

void ConsoleApp::find_book() {
    const auto number = console_.read_int("Book number: ");
    if (!number) {
        return;
    }
    const auto found = library_.find_book(*number);
    if (!found) {
        print_error(found.error());
    } else if (!found->has_value()) {
        console_.println("No book with number " + std::to_string(*number) + ".");
    } else {
        print_book_table(console_.out(), {**found});
    }
}

void ConsoleApp::update_book() {
    const auto number = console_.read_int("Number of the book to update: ");
    if (!number) {
        return;
    }
    const auto existing = library_.find_book(*number);
    if (!existing) {
        print_error(existing.error());
        return;
    }
    if (!existing->has_value()) {
        console_.println("No book with number " + std::to_string(*number) + ".");
        return;
    }
    const Book& book = **existing;

    console_.println("Press Enter to keep the current value.");
    const auto name = console_.read_line("Name [" + book.name() + "]: ");
    if (!name) {
        return;
    }
    const auto purpose = console_.read_line("Purpose [" + book.purpose() + "]: ");
    if (!purpose) {
        return;
    }
    report(library_.update_book(*number, keep_if_blank(*name, book.name()),
                                keep_if_blank(*purpose, book.purpose())),
           "Book updated.");
}

void ConsoleApp::delete_book() {
    const auto number = console_.read_int("Number of the book to delete: ");
    if (!number) {
        return;
    }
    const auto existing = library_.find_book(*number);
    if (!existing) {
        print_error(existing.error());
        return;
    }
    if (!existing->has_value()) {
        console_.println("No book with number " + std::to_string(*number) + ".");
        return;
    }
    if (!console_.confirm("Delete book " + std::to_string(*number) + " '" + (**existing).name() +
                          "'? (y/N): ")) {
        console_.println("Cancelled.");
        return;
    }
    report(library_.remove_book(*number), "Book deleted.");
}

// ----------------------------------------------------------------------------- students

void ConsoleApp::add_student() {
    const auto roll = console_.read_int("Roll number: ");
    if (!roll) {
        return;
    }
    const auto name = console_.read_line("Student name: ");
    if (!name) {
        return;
    }
    const auto school = console_.read_line("School name: ");
    if (!school) {
        return;
    }
    report(library_.add_student(*roll, *name, *school), "Student added.");
}

void ConsoleApp::list_students() {
    const auto students = library_.list_students();
    if (!students) {
        print_error(students.error());
    } else if (students->empty()) {
        console_.println("No students yet.");
    } else {
        print_student_table(console_.out(), *students);
    }
}

void ConsoleApp::find_student() {
    const auto roll = console_.read_int("Roll number: ");
    if (!roll) {
        return;
    }
    const auto found = library_.find_student(*roll);
    if (!found) {
        print_error(found.error());
    } else if (!found->has_value()) {
        console_.println("No student with roll number " + std::to_string(*roll) + ".");
    } else {
        print_student_table(console_.out(), {**found});
    }
}

void ConsoleApp::update_student() {
    const auto roll = console_.read_int("Roll number of the student to update: ");
    if (!roll) {
        return;
    }
    const auto existing = library_.find_student(*roll);
    if (!existing) {
        print_error(existing.error());
        return;
    }
    if (!existing->has_value()) {
        console_.println("No student with roll number " + std::to_string(*roll) + ".");
        return;
    }
    const Student& student = **existing;

    console_.println("Press Enter to keep the current value.");
    const auto name = console_.read_line("Name [" + student.name() + "]: ");
    if (!name) {
        return;
    }
    const auto school = console_.read_line("School [" + student.school() + "]: ");
    if (!school) {
        return;
    }
    report(library_.update_student(*roll, keep_if_blank(*name, student.name()),
                                   keep_if_blank(*school, student.school())),
           "Student updated.");
}

void ConsoleApp::delete_student() {
    const auto roll = console_.read_int("Roll number of the student to delete: ");
    if (!roll) {
        return;
    }
    const auto existing = library_.find_student(*roll);
    if (!existing) {
        print_error(existing.error());
        return;
    }
    if (!existing->has_value()) {
        console_.println("No student with roll number " + std::to_string(*roll) + ".");
        return;
    }
    if (!console_.confirm("Delete student " + std::to_string(*roll) + " '" + (**existing).name() +
                          "'? (y/N): ")) {
        console_.println("Cancelled.");
        return;
    }
    report(library_.remove_student(*roll), "Student deleted.");
}

// ---------------------------------------------------------------------- loans and backup

void ConsoleApp::issue_book() {
    const auto book = console_.read_int("Book number: ");
    if (!book) {
        return;
    }
    const auto roll = console_.read_int("Student roll number: ");
    if (!roll) {
        return;
    }
    report(library_.issue_book(*book, *roll), "Book " + std::to_string(*book) +
                                                  " issued to roll number " +
                                                  std::to_string(*roll) + ".");
}

void ConsoleApp::return_book() {
    const auto book = console_.read_int("Number of the book being returned: ");
    if (!book) {
        return;
    }
    report(library_.return_book(*book), "Book " + std::to_string(*book) + " returned.");
}

void ConsoleApp::show_issue_history() {
    const auto records = library_.list_issues();
    if (!records) {
        print_error(records.error());
        return;
    }
    if (records->empty()) {
        console_.println("No books have been issued yet.");
        return;
    }
    std::ostream& out = console_.out();
    out << '\n'
        << std::left << std::setw(10) << "BOOK NO" << std::setw(10) << "ROLL NO" << std::setw(22)
        << "STUDENT" << std::setw(14) << "ISSUED" << "RETURNED\n";
    for (const IssueRecord& record : *records) {
        out << std::setw(10) << record.book_no << std::setw(10) << record.roll_no << std::setw(22)
            << record.student_name << std::setw(14) << record.issued_on
            << record.returned_on.value_or("not yet") << '\n';
    }
}

void ConsoleApp::back_up() {
    const std::string file = backup_name_();
    report(library_.backup(file), "Backup saved to " + file);
}

// ------------------------------------------------------------------------------ helpers

void ConsoleApp::report(const Result<void>& result, const std::string& success_message) {
    if (result) {
        console_.println(success_message);
    } else {
        print_error(result.error());
    }
}

void ConsoleApp::print_error(const Error& error) {
    console_.println("Error: " + error.message);
}

} // namespace library
