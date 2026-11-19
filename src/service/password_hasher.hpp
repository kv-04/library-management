#pragma once

#include "domain/error.hpp"

#include <string>
#include <string_view>

namespace library {

// Turns a password into a safe-to-store "hash" and checks passwords against it.
// An interface, so the login rules can be tested with a trivial stand-in.
class PasswordHasher {
  public:
    virtual ~PasswordHasher() = default;

    [[nodiscard]] virtual Result<std::string> hash(std::string_view password) = 0;
    [[nodiscard]] virtual bool verify(const std::string& stored_hash,
                                      std::string_view password) = 0;
};

} // namespace library
