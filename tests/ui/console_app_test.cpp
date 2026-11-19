#include "service/password_hasher.hpp"
#include "storage/schema.hpp"
#include "storage/sqlite_repositories.hpp"
#include "ui/console_app.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

namespace library {
namespace {

class FakeHasher final : public PasswordHasher {
  public:
    Result<std::string> hash(std::string_view password) override {
        return "hashed:" + std::string(password);
    }
    bool verify(const std::string& stored_hash, std::string_view password) override {
        return stored_hash == "hashed:" + std::string(password);
    }
};

// Every line a person would type is one line of the script. The app runs against a real
// (in-memory) database; only the keyboard and screen are fake.
class ConsoleAppTest : public ::testing::Test {
  protected:
    ConsoleAppTest()
        : db_(sqlite::Database::open(":memory:").value()), books_(db_), students_(db_),
          issues_(db_), users_(db_), backup_(db_),
          library_(books_, students_, issues_, backup_, [] { return "2026-11-13"; }),
          auth_(users_, hasher_) {
        EXPECT_TRUE(create_schema(db_).has_value());
    }

    void create_admin() { ASSERT_TRUE(auth_.register_user("admin", "password123")); }

    // Runs the app with the given typed lines; returns everything it printed.
    std::string run(
        const std::string& typed_lines, int* exit_code = nullptr,
        ConsoleApp::NameProvider backup_name = [] { return std::string("unused.db"); }) {
        std::istringstream in(typed_lines);
        std::ostringstream out;
        Console console(in, out);
        ConsoleApp app(console, library_, auth_, std::move(backup_name));
        const int code = app.run();
        if (exit_code != nullptr) {
            *exit_code = code;
        }
        return out.str();
    }

    static bool contains(const std::string& text, const std::string& part) {
        return text.find(part) != std::string::npos;
    }

    static constexpr const char* kLogin = "admin\npassword123\n";

    sqlite::Database db_;
    SqliteBookRepository books_;
    SqliteStudentRepository students_;
    SqliteIssueRepository issues_;
    SqliteUserRepository users_;
    SqliteBackupStore backup_;
    FakeHasher hasher_;
    LibraryService library_;
    AuthService auth_;
};

// --------------------------------------------------------------------------- logging in

TEST_F(ConsoleAppTest, TheFirstRunCreatesTheAdministratorAccount) {
    int code = -1;

    const auto shown = run("admin\npassword123\npassword123\n0\n", &code);

    EXPECT_EQ(code, 0);
    EXPECT_TRUE(contains(shown, "First run"));
    EXPECT_TRUE(contains(shown, "Account created."));
    EXPECT_TRUE(auth_.log_in("admin", "password123").has_value());
}

TEST_F(ConsoleAppTest, TheFirstRunAsksAgainWhenThePasswordsDoNotMatch) {
    int code = -1;

    const auto shown = run("admin\npassword123\nsomething-else\n"
                           "admin\npassword123\npassword123\n0\n",
                           &code);

    EXPECT_EQ(code, 0);
    EXPECT_TRUE(contains(shown, "do not match"));
    EXPECT_TRUE(auth_.log_in("admin", "password123").has_value());
}

TEST_F(ConsoleAppTest, ACorrectLoginReachesTheMainMenu) {
    create_admin();
    int code = -1;

    const auto shown = run(std::string(kLogin) + "0\n", &code);

    EXPECT_EQ(code, 0);
    EXPECT_TRUE(contains(shown, "MAIN MENU"));
    EXPECT_TRUE(contains(shown, "Goodbye."));
}

TEST_F(ConsoleAppTest, ThreeWrongPasswordsEndTheProgramWithoutShowingTheMenu) {
    create_admin();
    int code = -1;

    const auto shown = run("admin\nnope1\nadmin\nnope2\nadmin\nnope3\n", &code);

    EXPECT_EQ(code, 1);
    EXPECT_TRUE(contains(shown, "wrong username or password"));
    EXPECT_TRUE(contains(shown, "Too many failed attempts."));
    EXPECT_FALSE(contains(shown, "MAIN MENU"));
}

// ------------------------------------------------------------------------- robustness

TEST_F(ConsoleAppTest, EndOfInputEndsTheProgramInsteadOfLoopingForever) {
    create_admin();
    int code = -1;

    run(kLogin, &code); // nothing typed after login

    EXPECT_EQ(code, 0);
}

TEST_F(ConsoleAppTest, ALetterInTheMenuIsRejectedAndTheMenuStillWorks) {
    create_admin();
    int code = -1;

    const auto shown = run(std::string(kLogin) + "abc\n9\n0\n", &code);

    EXPECT_EQ(code, 0);
    EXPECT_TRUE(contains(shown, "Please enter a whole number."));
    EXPECT_TRUE(contains(shown, "Please choose a number from the list."));
}

// ------------------------------------------------------------------------------ books

TEST_F(ConsoleAppTest, ABookCanBeAddedAndListed) {
    create_admin();

    const auto shown = run(std::string(kLogin) + "1\n"                           // Books
                                                 "1\n7\nThe Hobbit\nAdventure\n" // add
                                                 "2\n"                           // list
                                                 "0\n0\n");

    EXPECT_TRUE(contains(shown, "Book added."));
    EXPECT_TRUE(contains(shown, "The Hobbit"));
    EXPECT_TRUE(contains(shown, "Adventure"));
}

TEST_F(ConsoleAppTest, AddingADuplicateBookShowsAFriendlyError) {
    create_admin();

    const auto shown =
        run(std::string(kLogin) + "1\n1\n7\nDune\nSci-fi\n1\n7\nDune\nSci-fi\n0\n0\n");

    EXPECT_TRUE(contains(shown, "Error: book number 7 already exists"));
}

TEST_F(ConsoleAppTest, InvalidBookDetailsShowTheReasonAndSaveNothing) {
    create_admin();

    const auto shown = run(std::string(kLogin) + "1\n1\n7\n   \nSci-fi\n0\n0\n");

    EXPECT_TRUE(contains(shown, "Error: name must not be empty"));
    EXPECT_TRUE(library_.list_books()->empty());
}

TEST_F(ConsoleAppTest, UpdatingABookKeepsFieldsLeftBlank) {
    create_admin();
    ASSERT_TRUE(library_.add_book(7, "Dune", "Sci-fi"));

    const auto shown = run(std::string(kLogin) + "1\n4\n7\nDune Messiah\n\n0\n0\n");

    EXPECT_TRUE(contains(shown, "Book updated."));
    const auto book = library_.find_book(7);
    EXPECT_EQ((**book).name(), "Dune Messiah");
    EXPECT_EQ((**book).purpose(), "Sci-fi"); // left blank, so unchanged
}

TEST_F(ConsoleAppTest, DeletingABookNeedsConfirmation) {
    create_admin();
    ASSERT_TRUE(library_.add_book(7, "Dune", "Sci-fi"));

    const auto declined = run(std::string(kLogin) + "1\n5\n7\nn\n0\n0\n");
    EXPECT_TRUE(contains(declined, "Cancelled."));
    EXPECT_TRUE(library_.find_book(7)->has_value());

    const auto accepted = run(std::string(kLogin) + "1\n5\n7\ny\n0\n0\n");
    EXPECT_TRUE(contains(accepted, "Book deleted."));
    EXPECT_FALSE(library_.find_book(7)->has_value());
}

TEST_F(ConsoleAppTest, FindingAMissingBookSaysSo) {
    create_admin();

    const auto shown = run(std::string(kLogin) + "1\n3\n99\n0\n0\n");

    EXPECT_TRUE(contains(shown, "No book with number 99."));
}

// -------------------------------------------------------------------------- students

TEST_F(ConsoleAppTest, AStudentCanBeAddedListedAndDeleted) {
    create_admin();

    const auto shown = run(std::string(kLogin) + "2\n"
                                                 "1\n10\nAsha Rao\nWestview\n" // add
                                                 "2\n"                         // list
                                                 "5\n10\ny\n"                  // delete
                                                 "0\n0\n");

    EXPECT_TRUE(contains(shown, "Student added."));
    EXPECT_TRUE(contains(shown, "Asha Rao"));
    EXPECT_TRUE(contains(shown, "Student deleted."));
    EXPECT_TRUE(library_.list_students()->empty());
}

// ------------------------------------------------------------------------------ loans

TEST_F(ConsoleAppTest, ABookCanBeIssuedAndReturned) {
    create_admin();
    ASSERT_TRUE(library_.add_book(7, "Dune", "Sci-fi"));
    ASSERT_TRUE(library_.add_student(10, "Asha Rao", "Westview"));

    const auto shown = run(std::string(kLogin) + "3\n"
                                                 "1\n7\n10\n" // issue book 7 to roll 10
                                                 "3\n"        // history
                                                 "2\n7\n"     // return book 7
                                                 "3\n"        // history again
                                                 "0\n0\n");

    EXPECT_TRUE(contains(shown, "Book 7 issued to roll number 10."));
    EXPECT_TRUE(contains(shown, "not yet")); // first history: still out
    EXPECT_TRUE(contains(shown, "Book 7 returned."));
    EXPECT_TRUE(contains(shown, "Asha Rao"));
    EXPECT_TRUE(contains(shown, "2026-11-13"));
}

TEST_F(ConsoleAppTest, IssuingABookThatIsAlreadyOutShowsAFriendlyError) {
    create_admin();
    ASSERT_TRUE(library_.add_book(7, "Dune", "Sci-fi"));
    ASSERT_TRUE(library_.add_student(10, "Asha", "Westview"));

    const auto shown = run(std::string(kLogin) + "3\n1\n7\n10\n1\n7\n10\n0\n0\n");

    EXPECT_TRUE(contains(shown, "Error: book number 7 is already issued"));
}

TEST_F(ConsoleAppTest, IssuingAnUnknownBookShowsAFriendlyError) {
    create_admin();
    ASSERT_TRUE(library_.add_student(10, "Asha", "Westview"));

    const auto shown = run(std::string(kLogin) + "3\n1\n99\n10\n0\n0\n");

    EXPECT_TRUE(contains(shown, "Error: book number 99 does not exist"));
}

TEST_F(ConsoleAppTest, TheHistoryIsEmptyAtFirst) {
    create_admin();

    const auto shown = run(std::string(kLogin) + "3\n3\n0\n0\n");

    EXPECT_TRUE(contains(shown, "No books have been issued yet."));
}

// ------------------------------------------------------------------------------ backup

TEST_F(ConsoleAppTest, BackupWritesACopyOfTheData) {
    create_admin();
    ASSERT_TRUE(library_.add_book(7, "Dune", "Sci-fi"));
    const auto path =
        (std::filesystem::temp_directory_path() / "library_app_backup_test.db").string();
    std::filesystem::remove(path);

    const auto shown = run(std::string(kLogin) + "4\n0\n", nullptr, [&] { return path; });

    EXPECT_TRUE(contains(shown, "Backup saved to " + path));
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

TEST(DefaultBackupNameTest, LooksLikeADatedDatabaseFile) {
    const std::string name = default_backup_name();

    EXPECT_EQ(name.rfind("library-backup-", 0), 0U);
    EXPECT_EQ(name.substr(name.size() - 3), ".db");
}

} // namespace
} // namespace library
