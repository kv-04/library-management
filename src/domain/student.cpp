#include "domain/student.hpp"

#include "domain/text.hpp"

#include <utility>

namespace library {

Result<Student> Student::create(int roll_no, std::string name, std::string school) {
    if (roll_no <= 0) {
        return make_error(ErrorCode::InvalidInput, "roll number must be positive");
    }

    auto clean_name = clean_text("name", name, kMaxNameLength);
    if (!clean_name) {
        return tl::make_unexpected(clean_name.error());
    }
    auto clean_school = clean_text("school", school, kMaxSchoolLength);
    if (!clean_school) {
        return tl::make_unexpected(clean_school.error());
    }

    return Student(roll_no, std::move(*clean_name), std::move(*clean_school));
}

Student::Student(int roll_no, std::string name, std::string school)
    : roll_no_(roll_no), name_(std::move(name)), school_(std::move(school)) {}

} // namespace library
