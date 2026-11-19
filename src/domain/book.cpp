#include "domain/book.hpp"

#include "domain/text.hpp"

#include <utility>

namespace library {

Result<Book> Book::create(int book_no, std::string name, std::string purpose) {
    if (book_no <= 0) {
        return make_error(ErrorCode::InvalidInput, "book number must be positive");
    }

    auto clean_name = clean_text("name", name, kMaxTextLength);
    if (!clean_name) {
        return tl::make_unexpected(clean_name.error());
    }
    auto clean_purpose = clean_text("purpose", purpose, kMaxTextLength);
    if (!clean_purpose) {
        return tl::make_unexpected(clean_purpose.error());
    }

    return Book(book_no, std::move(*clean_name), std::move(*clean_purpose));
}

Book::Book(int book_no, std::string name, std::string purpose)
    : book_no_(book_no), name_(std::move(name)), purpose_(std::move(purpose)) {}

} // namespace library
