#include "ui/console.hpp"

#include <gtest/gtest.h>
#include <sstream>

namespace library {
namespace {

TEST(ConsoleTest, ReadLineShowsThePromptAndReturnsTheAnswer) {
    std::istringstream in("The Hobbit\n");
    std::ostringstream out;
    Console console(in, out);

    const auto line = console.read_line("Name: ");

    EXPECT_EQ(line, "The Hobbit");
    EXPECT_EQ(out.str(), "Name: ");
}

TEST(ConsoleTest, ReadLineReturnsNothingWhenTheInputEnds) {
    std::istringstream in("");
    std::ostringstream out;
    Console console(in, out);

    EXPECT_FALSE(console.read_line("Name: ").has_value());
}

TEST(ConsoleTest, WindowsLineEndingsAreRemoved) {
    std::istringstream in("Dune\r\n");
    std::ostringstream out;
    Console console(in, out);

    EXPECT_EQ(console.read_line(""), "Dune");
}

TEST(ConsoleTest, ReadIntAcceptsANumberWithSpacesAround) {
    std::istringstream in("  42 \n");
    std::ostringstream out;
    Console console(in, out);

    EXPECT_EQ(console.read_int("n: "), 42);
}

TEST(ConsoleTest, ReadIntKeepsAskingUntilTheAnswerIsAWholeNumber) {
    // The old program froze forever on this kind of input.
    std::istringstream in("abc\n\n12x\n3.5\n99999999999\n7\n");
    std::ostringstream out;
    Console console(in, out);

    EXPECT_EQ(console.read_int("n: "), 7);

    const std::string shown = out.str();
    std::size_t complaints = 0;
    for (std::size_t at = shown.find("Please enter a whole number."); at != std::string::npos;
         at = shown.find("Please enter a whole number.", at + 1)) {
        ++complaints;
    }
    EXPECT_EQ(complaints, 5U);
}

TEST(ConsoleTest, ReadIntAcceptsNegativeNumbers) {
    std::istringstream in("-5\n");
    std::ostringstream out;
    Console console(in, out);

    EXPECT_EQ(console.read_int("n: "), -5);
}

TEST(ConsoleTest, ReadIntStopsWhenTheInputEnds) {
    std::istringstream in("abc\n");
    std::ostringstream out;
    Console console(in, out);

    EXPECT_FALSE(console.read_int("n: ").has_value()); // asks once more, finds nothing, gives up
}

TEST(ConsoleTest, ConfirmOnlyAcceptsYes) {
    std::istringstream in("y\nYes\nn\n\nmaybe\n");
    std::ostringstream out;
    Console console(in, out);

    EXPECT_TRUE(console.confirm("ok? "));
    EXPECT_TRUE(console.confirm("ok? "));
    EXPECT_FALSE(console.confirm("ok? "));
    EXPECT_FALSE(console.confirm("ok? "));
    EXPECT_FALSE(console.confirm("ok? "));
    EXPECT_FALSE(console.confirm("ok? ")); // input ended
}

TEST(ConsoleTest, PasswordsAreReadLikeLinesWhenHidingIsOff) {
    std::istringstream in("secret123\n");
    std::ostringstream out;
    Console console(in, out, /*hide_password_echo=*/false);

    EXPECT_EQ(console.read_password("Password: "), "secret123");
}

} // namespace
} // namespace library
