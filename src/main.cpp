#include "service/auth_service.hpp"
#include "service/library_service.hpp"
#include "service/sodium_password_hasher.hpp"
#include "storage/schema.hpp"
#include "storage/sqlite.hpp"
#include "storage/sqlite_repositories.hpp"
#include "ui/console.hpp"
#include "ui/console_app.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr const char* kDefaultDatabase = "library.db";

void print_usage(const char* program) {
    std::cout << "Usage: " << program << " [database-file]\n"
              << "  database-file  where the data is kept (default: " << kDefaultDatabase << ")\n";
}

} // namespace

int main(int argc, char** argv) {
    using namespace library;

    if (argc > 2 || (argc == 2 && std::string(argv[1]) == "--help")) {
        print_usage(argv[0]);
        return argc > 2 ? 2 : 0;
    }
    const std::string database_path = argc == 2 ? argv[1] : kDefaultDatabase;

    // Open the storage.
    auto database = sqlite::Database::open(database_path);
    if (!database) {
        std::cerr << "Error: " << database.error().message << '\n';
        return 2;
    }
    if (const auto schema = create_schema(*database); !schema) {
        std::cerr << "Error: " << schema.error().message << '\n';
        return 2;
    }

    // Plug the pieces together: storage -> services -> screens.
    SqliteBookRepository books(*database);
    SqliteStudentRepository students(*database);
    SqliteIssueRepository issues(*database);
    SqliteUserRepository users(*database);
    SqliteBackupStore backups(*database);

    try {
        SodiumPasswordHasher hasher;
        LibraryService library(books, students, issues, backups);
        AuthService auth(users, hasher);
        Console console(std::cin, std::cout, /*hide_password_echo=*/true);
        std::cout << "Using database: " << database_path << '\n';
        ConsoleApp app(console, library, auth);
        return app.run();
    } catch (const std::exception& problem) {
        std::cerr << "Unexpected error: " << problem.what() << '\n';
        return 2;
    }
}
