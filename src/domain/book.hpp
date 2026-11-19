#pragma once

#include "domain/error.hpp"

#include <cstddef>
#include <string>

namespace library {

// A book in the library. A Book object is ALWAYS valid: the only way to get one is
// Book::create(), which checks the rules. No other code has to re-check them.
class Book {
  public:
    // Same 50-byte limit as the old char[50] fields, but now enforced instead of overflowing.
    static constexpr std::size_t kMaxTextLength = 50;

    [[nodiscard]] static Result<Book> create(int book_no, std::string name, std::string purpose);

    [[nodiscard]] int book_no() const noexcept { return book_no_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::string& purpose() const noexcept { return purpose_; }

    friend bool operator==(const Book&, const Book&) = default;

  private:
    Book(int book_no, std::string name, std::string purpose);

    int book_no_;
    std::string name_;
    std::string purpose_;
};

} // namespace library
