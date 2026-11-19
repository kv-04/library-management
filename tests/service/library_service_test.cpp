#include "service/library_service.hpp"
#include "storage/schema.hpp"
#include "storage/sqlite_repositories.hpp"

#include <gtest/gtest.h>
#include <string>

namespace library {
namespace {

bool mentions(const Result<void>& result, const std::string& text) {
    return !result && result.error().message.find(text) != std::string::npos;
}

class LibraryServiceTest : public ::testing::Test {
  protected:
    LibraryServiceTest()
        : db_(sqlite::Database::open(":memory:").value()), books_(db_), students_(db_),
          issues_(db_), backup_(db_),
          service_(books_, students_, issues_, backup_, [this] { return today_; }) {
        EXPECT_TRUE(create_schema(db_).has_value());
    }

    std::string today_ = "2026-11-13"; // tests can change "today" at will
    sqlite::Database db_;
    SqliteBookRepository books_;
    SqliteStudentRepository students_;
    SqliteIssueRepository issues_;
    SqliteBackupStore backup_;
    LibraryService service_;
};

// ------------------------------------------------------------------------------- books

TEST_F(LibraryServiceTest, AddsAndFindsABook) {
    ASSERT_TRUE(service_.add_book(1, " The Hobbit ", "Adventure"));

    const auto found = service_.find_book(1);

    ASSERT_TRUE(found.has_value() && found->has_value());
    EXPECT_EQ((**found).name(), "The Hobbit"); // validated and trimmed on the way in
}

TEST_F(LibraryServiceTest, RejectsAnInvalidBookWithoutSavingAnything) {
    const auto result = service_.add_book(1, "   ", "Adventure");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInput);
    EXPECT_TRUE(service_.list_books()->empty());
}

TEST_F(LibraryServiceTest, ADuplicateBookNumberGetsAFriendlyMessage) {
    ASSERT_TRUE(service_.add_book(7, "Dune", "Sci-fi"));

    const auto again = service_.add_book(7, "Other", "Other");

    ASSERT_FALSE(again.has_value());
    EXPECT_EQ(again.error().code, ErrorCode::AlreadyExists);
    EXPECT_EQ(again.error().message, "book number 7 already exists");
}

TEST_F(LibraryServiceTest, UpdatesAndRemovesBooks) {
    ASSERT_TRUE(service_.add_book(1, "Dune", "Sci-fi"));

    ASSERT_TRUE(service_.update_book(1, "Dune Messiah", "Sequel"));
    EXPECT_EQ((**service_.find_book(1)).name(), "Dune Messiah");

    ASSERT_TRUE(service_.remove_book(1));
    EXPECT_FALSE(service_.find_book(1)->has_value());
    EXPECT_EQ(service_.remove_book(1).error().code, ErrorCode::NotFound);
}

// ----------------------------------------------------------------------------- students

TEST_F(LibraryServiceTest, ManagesStudents) {
    ASSERT_TRUE(service_.add_student(10, "Asha Rao", "Westview"));
    EXPECT_EQ(service_.add_student(10, "Other", "Other").error().message,
              "student with roll number 10 already exists");

    ASSERT_TRUE(service_.update_student(10, "Asha Rao", "Hillcrest"));
    EXPECT_EQ((**service_.find_student(10)).school(), "Hillcrest");
    EXPECT_EQ(service_.list_students()->size(), 1U);

    ASSERT_TRUE(service_.remove_student(10));
    EXPECT_TRUE(service_.list_students()->empty());
}

TEST_F(LibraryServiceTest, RejectsAnInvalidStudent) {
    EXPECT_EQ(service_.add_student(0, "Asha", "Westview").error().code, ErrorCode::InvalidInput);
    EXPECT_EQ(service_.add_student(1, "", "Westview").error().code, ErrorCode::InvalidInput);
}

// -------------------------------------------------------------------------------- loans

TEST_F(LibraryServiceTest, IssuesABookAndRecordsTodaysDate) {
    ASSERT_TRUE(service_.add_book(1, "Dune", "Sci-fi"));
    ASSERT_TRUE(service_.add_student(10, "Asha", "Westview"));

    ASSERT_TRUE(service_.issue_book(1, 10));

    const auto all = service_.list_issues();
    ASSERT_TRUE(all.has_value());
    ASSERT_EQ(all->size(), 1U);
    EXPECT_EQ((*all)[0].issued_on, "2026-11-13");
    EXPECT_EQ((*all)[0].student_name, "Asha");
    EXPECT_FALSE((*all)[0].returned_on.has_value());
}

TEST_F(LibraryServiceTest, IssuingAnUnknownBookOrStudentSaysWhichOneIsMissing) {
    ASSERT_TRUE(service_.add_book(1, "Dune", "Sci-fi"));
    ASSERT_TRUE(service_.add_student(10, "Asha", "Westview"));

    const auto no_book = service_.issue_book(99, 10);
    EXPECT_EQ(no_book.error().code, ErrorCode::NotFound);
    EXPECT_TRUE(mentions(no_book, "book number 99"));

    const auto no_student = service_.issue_book(1, 99);
    EXPECT_EQ(no_student.error().code, ErrorCode::NotFound);
    EXPECT_TRUE(mentions(no_student, "roll number 99"));
}

TEST_F(LibraryServiceTest, ABookThatIsAlreadyOutCannotBeIssuedAgain) {
    ASSERT_TRUE(service_.add_book(1, "Dune", "Sci-fi"));
    ASSERT_TRUE(service_.add_student(10, "Asha", "Westview"));
    ASSERT_TRUE(service_.add_student(11, "Ben", "Westview"));
    ASSERT_TRUE(service_.issue_book(1, 10));

    const auto second = service_.issue_book(1, 11);

    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error().code, ErrorCode::AlreadyExists);
    EXPECT_EQ(second.error().message, "book number 1 is already issued");
}

TEST_F(LibraryServiceTest, ReturningABookRecordsTheDateAndFreesTheBook) {
    ASSERT_TRUE(service_.add_book(1, "Dune", "Sci-fi"));
    ASSERT_TRUE(service_.add_student(10, "Asha", "Westview"));
    ASSERT_TRUE(service_.add_student(11, "Ben", "Westview"));
    ASSERT_TRUE(service_.issue_book(1, 10));

    today_ = "2026-11-20";
    ASSERT_TRUE(service_.return_book(1));
    ASSERT_TRUE(service_.issue_book(1, 11)); // free again

    const auto all = service_.list_issues();
    ASSERT_EQ(all->size(), 2U);
    EXPECT_EQ((*all)[1].returned_on, "2026-11-20");
}

TEST_F(LibraryServiceTest, ReturningABookThatIsNotOutSaysSo) {
    ASSERT_TRUE(service_.add_book(1, "Dune", "Sci-fi"));

    const auto result = service_.return_book(1);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message, "book number 1 is not currently issued");
}

TEST_F(LibraryServiceTest, ItemsWithLoanHistoryCannotBeDeleted) {
    ASSERT_TRUE(service_.add_book(1, "Dune", "Sci-fi"));
    ASSERT_TRUE(service_.add_student(10, "Asha", "Westview"));
    ASSERT_TRUE(service_.issue_book(1, 10));

    EXPECT_TRUE(mentions(service_.remove_book(1), "loan history"));
    EXPECT_TRUE(mentions(service_.remove_student(10), "loan history"));
}

TEST(TodaysDateTest, HasTheFormatYearMonthDay) {
    const std::string today = todays_date();

    ASSERT_EQ(today.size(), 10U);
    EXPECT_EQ(today[4], '-');
    EXPECT_EQ(today[7], '-');
}

} // namespace
} // namespace library
