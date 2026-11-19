#pragma once

#include "domain/error.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace library {

// Removes spaces, tabs and newlines from both ends of the text.
[[nodiscard]] std::string trim(std::string_view text);

// Removes spaces/tabs/newlines from both ends of `raw`, then checks the result is not empty
// and not longer than `max_length` bytes. `field` is the name used in the error message,
// e.g. "name must not be empty".
[[nodiscard]] Result<std::string> clean_text(std::string_view field, std::string_view raw,
                                             std::size_t max_length);

} // namespace library
