#include "service/sodium_password_hasher.hpp"

#include <array>
#include <sodium.h>
#include <stdexcept>

namespace library {

SodiumPasswordHasher::SodiumPasswordHasher(HashStrength strength) : strength_(strength) {
    // sodium_init() returns 0 on first success, 1 if already initialized, -1 on failure.
    if (sodium_init() < 0) {
        throw std::runtime_error("could not initialize the password library");
    }
}

Result<std::string> SodiumPasswordHasher::hash(std::string_view password) {
    const bool strong = strength_ == HashStrength::Normal;
    const unsigned long long operations =
        strong ? crypto_pwhash_OPSLIMIT_INTERACTIVE : crypto_pwhash_OPSLIMIT_MIN;
    const std::size_t memory =
        strong ? crypto_pwhash_MEMLIMIT_INTERACTIVE : crypto_pwhash_MEMLIMIT_MIN;

    std::array<char, crypto_pwhash_STRBYTES> output{};
    if (crypto_pwhash_str(output.data(), password.data(), password.size(), operations, memory) !=
        0) {
        return make_error(ErrorCode::StorageFailure,
                          "could not hash the password (out of memory?)");
    }
    return std::string(output.data());
}

bool SodiumPasswordHasher::verify(const std::string& stored_hash, std::string_view password) {
    return crypto_pwhash_str_verify(stored_hash.c_str(), password.data(), password.size()) == 0;
}

} // namespace library
