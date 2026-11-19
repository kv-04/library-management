#include "domain/book.hpp"

#include <gtest/gtest.h>
#include <string>

namespace library {
namespace {

TEST(BookTest, CreatesABookFromValidInput) {
    const auto book = Book::create(7, "The Hobbit", "Adventure");

    ASSERT_TRUE(book.has_value());
    EXPECT_EQ(book->book_no(), 7);
    EXPECT_EQ(book->name(), "The Hobbit"); // spaces inside names are fine now
    EXPECT_EQ(book->purpose(), "Adventure");
}

TEST(BookTest, TrimsSurroundingWhitespace) {
    const auto book = Book::create(1, "  Dune \n", "\tSci-fi  ");

    ASSERT_TRUE(book.has_value());
    EXPECT_EQ(book->name(), "Dune");
    EXPECT_EQ(book->purpose(), "Sci-fi");
}

TEST(BookTest, RejectsNonPositiveBookNumbers) {
    for (const int bad : {0, -1, -500}) {
        const auto book = Book::create(bad, "Dune", "Sci-fi");
        ASSERT_FALSE(book.has_value()) << "book_no " << bad;
        EXPECT_EQ(book.error().code, ErrorCode::InvalidInput);
    }
}

TEST(BookTest, RejectsBlankName) {
    const auto book = Book::create(1, "   ", "Sci-fi");

    ASSERT_FALSE(book.has_value());
    EXPECT_EQ(book.error().code, ErrorCode::InvalidInput);
    EXPECT_NE(book.error().message.find("name"), std::string::npos);
}

TEST(BookTest, RejectsBlankPurpose) {
    const auto book = Book::create(1, "Dune", "");

    ASSERT_FALSE(book.has_value());
    EXPECT_NE(book.error().message.find("purpose"), std::string::npos);
}

TEST(BookTest, EnforcesTheLengthLimitExactly) {
    const std::string at_limit(Book::kMaxTextLength, 'a');
    const std::string over_limit(Book::kMaxTextLength + 1, 'a');

    EXPECT_TRUE(Book::create(1, at_limit, "ok").has_value());
    EXPECT_FALSE(Book::create(1, over_limit, "ok").has_value());
    EXPECT_FALSE(Book::create(1, "ok", over_limit).has_value());
}

TEST(BookTest, BooksWithTheSameFieldsAreEqual) {
    const auto a = Book::create(1, "Dune", "Sci-fi");
    const auto b = Book::create(1, "Dune", "Sci-fi");
    const auto c = Book::create(2, "Dune", "Sci-fi");

    ASSERT_TRUE(a && b && c);
    EXPECT_EQ(*a, *b);
    EXPECT_NE(*a, *c);
}

} // namespace
} // namespace library
