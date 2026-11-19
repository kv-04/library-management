#include "domain/error.hpp"

#include <utility>

namespace library {

std::string_view to_string(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::InvalidInput:
        return "invalid input";
    case ErrorCode::NotFound:
        return "not found";
    case ErrorCode::AlreadyExists:
        return "already exists";
    case ErrorCode::Conflict:
        return "conflict";
    case ErrorCode::Unauthorized:
        return "unauthorized";
    case ErrorCode::StorageFailure:
        return "storage failure";
    }
    return "unknown"; // unreachable unless a new enumerator is added without updating this
}

tl::unexpected<Error> make_error(ErrorCode code, std::string message) {
    return tl::unexpected<Error>(Error{code, std::move(message)});
}

} // namespace library
