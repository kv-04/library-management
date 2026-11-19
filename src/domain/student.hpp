#pragma once

#include "domain/error.hpp"

#include <cstddef>
#include <string>

namespace library {

// A student who can borrow books. Like Book, a Student is ALWAYS valid:
// the only way to get one is Student::create().
class Student {
  public:
    // Same 20-byte limits as the old char[20] fields, but enforced instead of overflowing.
    static constexpr std::size_t kMaxNameLength = 20;
    static constexpr std::size_t kMaxSchoolLength = 20;

    [[nodiscard]] static Result<Student> create(int roll_no, std::string name, std::string school);

    [[nodiscard]] int roll_no() const noexcept { return roll_no_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::string& school() const noexcept { return school_; }

    friend bool operator==(const Student&, const Student&) = default;

  private:
    Student(int roll_no, std::string name, std::string school);

    int roll_no_;
    std::string name_;
    std::string school_;
};

} // namespace library
