#pragma once

#include "domain/error.hpp"
#include "storage/sqlite.hpp"

namespace library {

// Creates the tables if they are not there yet. Safe to call every time the program starts.
[[nodiscard]] Result<void> create_schema(sqlite::Database& db);

} // namespace library
