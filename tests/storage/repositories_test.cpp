#include "storage/schema.hpp"
#include "storage/sqlite_repositories.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <string>

namespace library {
namespace {

// Every test gets its own fresh in-memory database with all tables created.
class RepositoriesTest : public ::testing::Test {
  protected:
    RepositoriesTest()
        : db_(sqlite::Database::open(":memory:").value()), books_(db_), students_(db_),
          issues_(db_), users_(db_), backup_(db_) {
        EXPECT_TRUE(create_schema(db_).has_value());
    }

    Book book(int number, const std::string& name = "Dune") {
        return Book::create(number, name, "Sci-fi").value();
    }
    Student student(int roll, const std::string& name = "Asha") {
        return Student::create(roll, name, "Westview").value();
    }

    sqlite::Database db_;
    SqliteBookRepository books_;
    SqliteStudentRepository students_;
    SqliteIssueRepository issues_;
    SqliteUserRepository users_;
    SqliteBackupStore backup_;
};

// ----------------------------------------------------------------------------- books

TEST_F(RepositoriesTest, AddedBookCanBeFound) {
    ASSERT_TRUE(books_.add(book(7, "The Hobbit")));

    const auto found = books_.find(7);

    ASSERT_TRUE(found.has_value());
    ASSERT_TRUE(found->has_value());
    EXPECT_EQ(**found, book(7, "The Hobbit"));
}

TEST_F(RepositoriesTest, FindingAMissingBookIsNotAnError) {
    const auto found = books_.find(99);

    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->has_value());
}

TEST_F(RepositoriesTest, AddingADuplicateBookNumberIsRejected) {
    ASSERT_TRUE(books_.add(book(1)));

    const auto again = books_.add(book(1, "Other"));

    ASSERT_FALSE(again.has_value());
    EXPECT_EQ(again.error().code, ErrorCode::AlreadyExists);
}

TEST_F(RepositoriesTest, UpdateChangesNameAndPurpose) {
    ASSERT_TRUE(books_.add(book(1)));

    ASSERT_TRUE(books_.update(Book::create(1, "Dune Messiah", "Sequel").value()));

    EXPECT_EQ((**books_.find(1)).name(), "Dune Messiah");
    EXPECT_EQ((**books_.find(1)).purpose(), "Sequel");
}

TEST_F(RepositoriesTest, UpdatingAMissingBookReportsNotFound) {
    const auto result = books_.update(book(5));

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::NotFound);
}

TEST_F(RepositoriesTest, RemoveDeletesTheBook) {
    ASSERT_TRUE(books_.add(book(1)));

    ASSERT_TRUE(books_.remove(1));

    EXPECT_FALSE(books_.find(1)->has_value());
}

TEST_F(RepositoriesTest, RemovingAMissingBookReportsNotFound) {
    const auto result = books_.remove(1);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::NotFound);
}

TEST_F(RepositoriesTest, BooksAreListedInNumberOrder) {
    ASSERT_TRUE(books_.add(book(30)));
    ASSERT_TRUE(books_.add(book(10)));
    ASSERT_TRUE(books_.add(book(20)));

    const auto all = books_.list();

    ASSERT_TRUE(all.has_value());
    ASSERT_EQ(all->size(), 3U);
    EXPECT_EQ((*all)[0].book_no(), 10);
    EXPECT_EQ((*all)[1].book_no(), 20);
    EXPECT_EQ((*all)[2].book_no(), 30);
}

// --------------------------------------------------------------------------- students

TEST_F(RepositoriesTest, StudentsCanBeAddedFoundUpdatedListedAndRemoved) {
    ASSERT_TRUE(students_.add(student(2, "Ben")));
    ASSERT_TRUE(students_.add(student(1, "Asha")));

    EXPECT_EQ(**students_.find(1), student(1, "Asha"));
    EXPECT_FALSE(students_.find(9)->has_value());

    ASSERT_TRUE(students_.update(Student::create(1, "Asha Rao", "Hillcrest").value()));
    EXPECT_EQ((**students_.find(1)).school(), "Hillcrest");

    const auto all = students_.list();
    ASSERT_TRUE(all.has_value());
    ASSERT_EQ(all->size(), 2U);
    EXPECT_EQ((*all)[0].roll_no(), 1);

    ASSERT_TRUE(students_.remove(2));
    EXPECT_EQ(students_.list()->size(), 1U);
}

TEST_F(RepositoriesTest, StudentErrorsUseTheSameCodesAsBooks) {
    ASSERT_TRUE(students_.add(student(1)));

    EXPECT_EQ(students_.add(student(1)).error().code, ErrorCode::AlreadyExists);
    EXPECT_EQ(students_.update(student(9)).error().code, ErrorCode::NotFound);
    EXPECT_EQ(students_.remove(9).error().code, ErrorCode::NotFound);
}

// ----------------------------------------------------------------------------- issues

TEST_F(RepositoriesTest, ABookCanBeIssuedToAStudent) {
    ASSERT_TRUE(books_.add(book(1)));
    ASSERT_TRUE(students_.add(student(10, "Asha")));

    ASSERT_TRUE(issues_.add_issue(1, 10, "2026-11-13"));

    const auto all = issues_.list();
    ASSERT_TRUE(all.has_value());
    ASSERT_EQ(all->size(), 1U);
    EXPECT_EQ((*all)[0].book_no, 1);
    EXPECT_EQ((*all)[0].roll_no, 10);
    EXPECT_EQ((*all)[0].student_name, "Asha"); // name comes from the students table
    EXPECT_EQ((*all)[0].issued_on, "2026-11-13");
    EXPECT_FALSE((*all)[0].returned_on.has_value());
}

TEST_F(RepositoriesTest, ABookCannotBeIssuedTwiceAtTheSameTime) {
    ASSERT_TRUE(books_.add(book(1)));
    ASSERT_TRUE(students_.add(student(10)));
    ASSERT_TRUE(students_.add(student(11, "Ben")));
    ASSERT_TRUE(issues_.add_issue(1, 10, "2026-11-13"));

    const auto second = issues_.add_issue(1, 11, "2026-11-14");

    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error().code, ErrorCode::AlreadyExists);
}

TEST_F(RepositoriesTest, ABookCanBeIssuedAgainAfterItIsReturned) {
    ASSERT_TRUE(books_.add(book(1)));
    ASSERT_TRUE(students_.add(student(10)));
    ASSERT_TRUE(issues_.add_issue(1, 10, "2026-11-13"));

    ASSERT_TRUE(issues_.mark_returned(1, "2026-11-20"));
    ASSERT_TRUE(issues_.add_issue(1, 10, "2026-11-21"));

    const auto all = issues_.list();
    ASSERT_EQ(all->size(), 2U);
    EXPECT_FALSE((*all)[0].returned_on.has_value()); // newest first, still out
    EXPECT_EQ((*all)[1].returned_on, "2026-11-20");  // the older one was returned
}

TEST_F(RepositoriesTest, IssuingAnUnknownBookOrStudentIsRejected) {
    ASSERT_TRUE(books_.add(book(1)));
    ASSERT_TRUE(students_.add(student(10)));

    EXPECT_EQ(issues_.add_issue(99, 10, "2026-11-13").error().code, ErrorCode::Conflict);
    EXPECT_EQ(issues_.add_issue(1, 99, "2026-11-13").error().code, ErrorCode::Conflict);
}

TEST_F(RepositoriesTest, ReturningABookThatIsNotOutReportsNotFound) {
    ASSERT_TRUE(books_.add(book(1)));

    const auto result = issues_.mark_returned(1, "2026-11-20");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::NotFound);
}

TEST_F(RepositoriesTest, BooksAndStudentsWithLoanHistoryCannotBeDeleted) {
    ASSERT_TRUE(books_.add(book(1)));
    ASSERT_TRUE(students_.add(student(10)));
    ASSERT_TRUE(issues_.add_issue(1, 10, "2026-11-13"));

    EXPECT_EQ(books_.remove(1).error().code, ErrorCode::Conflict);
    EXPECT_EQ(students_.remove(10).error().code, ErrorCode::Conflict);
}

// ------------------------------------------------------------------------------ users

TEST_F(RepositoriesTest, UsersCanBeStoredAndLookedUp) {
    EXPECT_EQ(*users_.count(), 0);

    ASSERT_TRUE(users_.add("admin", "hash-value"));

    EXPECT_EQ(*users_.count(), 1);
    EXPECT_EQ(**users_.find_password_hash("admin"), "hash-value");
    EXPECT_FALSE(users_.find_password_hash("nobody")->has_value());
    EXPECT_EQ(users_.add("admin", "x").error().code, ErrorCode::AlreadyExists);
}

// ----------------------------------------------------------------------------- backup

TEST_F(RepositoriesTest, BackupCreatesACompleteCopyAndNeverOverwrites) {
    ASSERT_TRUE(books_.add(book(1, "Dune")));
    const auto path = (std::filesystem::temp_directory_path() / "library_backup_test.db").string();
    std::filesystem::remove(path);

    ASSERT_TRUE(backup_.save_copy(path));

    auto copy = sqlite::Database::open(path).value();
    SqliteBookRepository copied_books(copy);
    EXPECT_EQ((**copied_books.find(1)).name(), "Dune");

    const auto again = backup_.save_copy(path); // the file now exists
    EXPECT_FALSE(again.has_value());

    std::filesystem::remove(path);
}

} // namespace
} // namespace library
