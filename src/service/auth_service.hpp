#pragma once

#include "domain/repositories.hpp"
#include "service/password_hasher.hpp"

#include <cstddef>
#include <string>

namespace library {

// Creating accounts and logging in. Passwords are never stored, only their hashes.
class AuthService {
  public:
    static constexpr std::size_t kMaxUsernameLength = 30;
    static constexpr std::size_t kMinPasswordLength = 8;
    static constexpr std::size_t kMaxPasswordLength = 128;

    AuthService(UserRepository& users, PasswordHasher& hasher);

    // True once at least one account exists (false on the very first run).
    [[nodiscard]] Result<bool> has_users();

    [[nodiscard]] Result<void> register_user(const std::string& username,
                                             const std::string& password);

    // Unauthorized for an unknown user AND for a wrong password, with the same message,
    // so an attacker cannot learn which usernames exist.
    [[nodiscard]] Result<void> log_in(const std::string& username, const std::string& password);

  private:
    UserRepository& users_;
    PasswordHasher& hasher_;
};

} // namespace library
