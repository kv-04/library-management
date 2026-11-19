#pragma once

#include <string>
#include <string_view>
#include <tl/expected.hpp>

namespace library {

// What went wrong, in a form code can switch on (messages are for humans).
enum class ErrorCode {
    InvalidInput,  // the caller gave us something that breaks a rule
    NotFound,      // the thing asked for does not exist
    AlreadyExists, // creating something that is already there
    Conflict,      // the action clashes with current state (e.g. book already issued)
    Unauthorized,  // wrong username or password
    StorageFailure // the database or file system failed
};

struct Error {
    ErrorCode code;
    std::string message;

    friend bool operator==(const Error&, const Error&) = default;
};

// "Either a T, or an Error". Used by every operation that can fail for a reason
// the caller should know about.
//
// tl::expected has the same interface as C++23's std::expected but works with C++20
// toolchains. Moving to std::expected later means changing only this alias and make_error().
template <typename T> using Result = tl::expected<T, Error>;

[[nodiscard]] std::string_view to_string(ErrorCode code) noexcept;

// Shorthand for `return tl::unexpected(Error{code, message});`
[[nodiscard]] tl::unexpected<Error> make_error(ErrorCode code, std::string message);

} // namespace library
