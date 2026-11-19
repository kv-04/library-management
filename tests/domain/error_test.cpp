#include "domain/error.hpp"

#include <gtest/gtest.h>

namespace library {
namespace {

TEST(ErrorTest, EveryCodeHasAReadableName) {
    EXPECT_EQ(to_string(ErrorCode::NotFound), "not found");
    EXPECT_EQ(to_string(ErrorCode::AlreadyExists), "already exists");
    EXPECT_EQ(to_string(ErrorCode::InvalidInput), "invalid input");
}

TEST(ErrorTest, MakeErrorProducesAFailedResult) {
    const Result<int> result = make_error(ErrorCode::NotFound, "no such book");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::NotFound);
    EXPECT_EQ(result.error().message, "no such book");
}

TEST(ErrorTest, SuccessfulResultHoldsTheValue) {
    const Result<int> result = 42;

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

} // namespace
} // namespace library
