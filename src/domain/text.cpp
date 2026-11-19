#include "domain/text.hpp"

#include <algorithm>
#include <cctype>

namespace library {

namespace {

bool is_space(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

} // namespace

std::string trim(std::string_view text) {
    const auto first = std::find_if_not(text.begin(), text.end(), is_space);
    if (first == text.end()) {
        return {};
    }
    const auto last = std::find_if_not(text.rbegin(), text.rend(), is_space).base();
    return std::string(first, last);
}

Result<std::string> clean_text(std::string_view field, std::string_view raw,
                               std::size_t max_length) {
    std::string text = trim(raw);
    if (text.empty()) {
        return make_error(ErrorCode::InvalidInput, std::string(field) + " must not be empty");
    }
    if (text.size() > max_length) {
        return make_error(ErrorCode::InvalidInput, std::string(field) + " must be at most " +
                                                       std::to_string(max_length) + " characters");
    }
    return text;
}

} // namespace library
