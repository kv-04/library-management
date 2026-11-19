#include "service/auth_service.hpp"

#include "domain/text.hpp"

namespace library {

AuthService::AuthService(UserRepository& users, PasswordHasher& hasher)
    : users_(users), hasher_(hasher) {}

Result<bool> AuthService::has_users() {
    auto count = users_.count();
    if (!count) {
        return tl::make_unexpected(count.error());
    }
    return *count > 0;
}

Result<void> AuthService::register_user(const std::string& username, const std::string& password) {
    auto clean_name = clean_text("username", username, kMaxUsernameLength);
    if (!clean_name) {
        return tl::make_unexpected(clean_name.error());
    }
    if (password.size() < kMinPasswordLength) {
        return make_error(ErrorCode::InvalidInput, "password must be at least " +
                                                       std::to_string(kMinPasswordLength) +
                                                       " characters");
    }
    if (password.size() > kMaxPasswordLength) {
        return make_error(ErrorCode::InvalidInput, "password must be at most " +
                                                       std::to_string(kMaxPasswordLength) +
                                                       " characters");
    }

    auto hashed = hasher_.hash(password);
    if (!hashed) {
        return tl::make_unexpected(hashed.error());
    }

    auto added = users_.add(*clean_name, *hashed);
    if (!added && added.error().code == ErrorCode::AlreadyExists) {
        return make_error(ErrorCode::AlreadyExists, "username '" + *clean_name + "' is taken");
    }
    return added;
}

Result<void> AuthService::log_in(const std::string& username, const std::string& password) {
    const auto wrong = [] {
        return make_error(ErrorCode::Unauthorized, "wrong username or password");
    };

    auto clean_name = clean_text("username", username, kMaxUsernameLength);
    if (!clean_name) {
        return wrong();
    }
    auto stored = users_.find_password_hash(*clean_name);
    if (!stored) {
        return tl::make_unexpected(stored.error());
    }
    if (!stored->has_value() || !hasher_.verify(**stored, password)) {
        return wrong();
    }
    return {};
}

} // namespace library
