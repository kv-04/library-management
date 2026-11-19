#pragma once

#include "service/password_hasher.hpp"

namespace library {

enum class HashStrength {
    Normal,      // the real setting: deliberately slow, so guessing passwords is expensive
    WeakForTests // fast and weak: only for unit tests
};

// Password hashing with libsodium's Argon2id. Each hash includes its own random salt, so the
// same password hashes differently every time.
class SodiumPasswordHasher final : public PasswordHasher {
  public:
    // Throws std::runtime_error if libsodium cannot start (very rare).
    explicit SodiumPasswordHasher(HashStrength strength = HashStrength::Normal);

    Result<std::string> hash(std::string_view password) override;
    bool verify(const std::string& stored_hash, std::string_view password) override;

  private:
    HashStrength strength_;
};

} // namespace library
