#include "domain/text.hpp"

#include <gtest/gtest.h>
#include <string>

namespace library {
namespace {

TEST(CleanTextTest, ReturnsTheTextUnchangedWhenItIsAlreadyClean) {
    const auto result = clean_text("name", "Dune", 10);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Dune");
}

TEST(CleanTextTest, RemovesWhitespaceFromBothEnds) {
    const auto result = clean_text("name", " \t Dune \n", 10);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Dune");
}

TEST(CleanTextTest, KeepsSpacesInsideTheText) {
    const auto result = clean_text("name", "  The Hobbit ", 20);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "The Hobbit");
}

TEST(CleanTextTest, RejectsEmptyAndWhitespaceOnlyText) {
    for (const char* bad : {"", "   ", "\t\n"}) {
        const auto result = clean_text("school", bad, 10);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code, ErrorCode::InvalidInput);
        EXPECT_NE(result.error().message.find("school"), std::string::npos);
    }
}

TEST(CleanTextTest, AcceptsTextExactlyAtTheLimitAndRejectsOneOver) {
    EXPECT_TRUE(clean_text("name", std::string(10, 'a'), 10).has_value());

    const auto too_long = clean_text("name", std::string(11, 'a'), 10);
    ASSERT_FALSE(too_long.has_value());
    EXPECT_NE(too_long.error().message.find("10"), std::string::npos);
}

TEST(CleanTextTest, MeasuresTheLimitAfterTrimming) {
    // 10 letters plus padding is fine: only the trimmed text counts.
    EXPECT_TRUE(clean_text("name", "   aaaaaaaaaa   ", 10).has_value());
}

TEST(TrimTest, RemovesWhitespaceFromBothEndsOnly) {
    EXPECT_EQ(trim("  a b \n"), "a b");
    EXPECT_EQ(trim("abc"), "abc");
    EXPECT_EQ(trim("   "), "");
    EXPECT_EQ(trim(""), "");
}

} // namespace
} // namespace library
