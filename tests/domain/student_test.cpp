#include "domain/student.hpp"

#include <gtest/gtest.h>
#include <string>

namespace library {
namespace {

TEST(StudentTest, CreatesAStudentFromValidInput) {
    const auto student = Student::create(12, "Asha Rao", "Westview");

    ASSERT_TRUE(student.has_value());
    EXPECT_EQ(student->roll_no(), 12);
    EXPECT_EQ(student->name(), "Asha Rao");
    EXPECT_EQ(student->school(), "Westview");
}

TEST(StudentTest, TrimsSurroundingWhitespace) {
    const auto student = Student::create(1, "  Asha ", "\tWestview\n");

    ASSERT_TRUE(student.has_value());
    EXPECT_EQ(student->name(), "Asha");
    EXPECT_EQ(student->school(), "Westview");
}

TEST(StudentTest, RejectsNonPositiveRollNumbers) {
    for (const int bad : {0, -1, -77}) {
        const auto student = Student::create(bad, "Asha", "Westview");
        ASSERT_FALSE(student.has_value()) << "roll_no " << bad;
        EXPECT_EQ(student.error().code, ErrorCode::InvalidInput);
    }
}

TEST(StudentTest, RejectsBlankNameOrSchool) {
    const auto no_name = Student::create(1, " ", "Westview");
    ASSERT_FALSE(no_name.has_value());
    EXPECT_NE(no_name.error().message.find("name"), std::string::npos);

    const auto no_school = Student::create(1, "Asha", "");
    ASSERT_FALSE(no_school.has_value());
    EXPECT_NE(no_school.error().message.find("school"), std::string::npos);
}

TEST(StudentTest, EnforcesTheLengthLimitsExactly) {
    const std::string name_at_limit(Student::kMaxNameLength, 'a');
    const std::string name_over(Student::kMaxNameLength + 1, 'a');
    const std::string school_at_limit(Student::kMaxSchoolLength, 'b');
    const std::string school_over(Student::kMaxSchoolLength + 1, 'b');

    EXPECT_TRUE(Student::create(1, name_at_limit, school_at_limit).has_value());
    EXPECT_FALSE(Student::create(1, name_over, "ok").has_value());
    EXPECT_FALSE(Student::create(1, "ok", school_over).has_value());
}

TEST(StudentTest, StudentsWithTheSameFieldsAreEqual) {
    const auto a = Student::create(1, "Asha", "Westview");
    const auto b = Student::create(1, "Asha", "Westview");
    const auto c = Student::create(2, "Asha", "Westview");

    ASSERT_TRUE(a && b && c);
    EXPECT_EQ(*a, *b);
    EXPECT_NE(*a, *c);
}

} // namespace
} // namespace library
