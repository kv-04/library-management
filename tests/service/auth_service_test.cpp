#include "service/auth_service.hpp"
#include "service/sodium_password_hasher.hpp"
#include "storage/schema.hpp"
#include "storage/sqlite_repositories.hpp"

#include <gtest/gtest.h>
#include <string>

namespace library {
namespace {

// A stand-in that is trivial and instant, so the login RULES can be tested on their own.
class FakeHasher final : public PasswordHasher {
  public:
    Result<std::string> hash(std::string_view password) override {
        return "hashed:" + std::string(password);
    }
    bool verify(const std::string& stored_hash, std::string_view password) override {
        return stored_hash == "hashed:" + std::string(password);
    }
};

class AuthServiceTest : public ::testing::Test {
  protected:
    AuthServiceTest()
        : db_(sqlite::Database::open(":memory:").value()), users_(db_), auth_(users_, hasher_) {
        EXPECT_TRUE(create_schema(db_).has_value());
    }

    sqlite::Database db_;
    SqliteUserRepository users_;
    FakeHasher hasher_;
    AuthService auth_;
};

TEST_F(AuthServiceTest, ThereAreNoUsersOnTheFirstRun) {
    EXPECT_EQ(auth_.has_users().value(), false);

    ASSERT_TRUE(auth_.register_user("admin", "correct horse"));

    EXPECT_EQ(auth_.has_users().value(), true);
}

TEST_F(AuthServiceTest, ARegisteredUserCanLogIn) {
    ASSERT_TRUE(auth_.register_user("admin", "correct horse"));

    EXPECT_TRUE(auth_.log_in("admin", "correct horse").has_value());
    EXPECT_TRUE(auth_.log_in("  admin ", "correct horse").has_value()); // spaces around the name
}

TEST_F(AuthServiceTest, ThePasswordItselfIsNeverStored) {
    ASSERT_TRUE(auth_.register_user("admin", "correct horse"));

    const auto stored = users_.find_password_hash("admin");

    ASSERT_TRUE(stored.has_value() && stored->has_value());
    EXPECT_NE(**stored, "correct horse");
}

TEST_F(AuthServiceTest, WrongPasswordAndUnknownUserGetTheSameError) {
    ASSERT_TRUE(auth_.register_user("admin", "correct horse"));

    const auto wrong_password = auth_.log_in("admin", "battery staple");
    const auto unknown_user = auth_.log_in("nobody", "correct horse");
    const auto blank_user = auth_.log_in("   ", "correct horse");

    ASSERT_FALSE(wrong_password.has_value());
    ASSERT_FALSE(unknown_user.has_value());
    ASSERT_FALSE(blank_user.has_value());
    EXPECT_EQ(wrong_password.error().code, ErrorCode::Unauthorized);
    EXPECT_EQ(wrong_password.error(), unknown_user.error());
    EXPECT_EQ(wrong_password.error(), blank_user.error());
}

TEST_F(AuthServiceTest, RejectsWeakOrInvalidRegistrations) {
    EXPECT_EQ(auth_.register_user("admin", "short").error().code, ErrorCode::InvalidInput);
    EXPECT_EQ(auth_.register_user("admin", std::string(129, 'x')).error().code,
              ErrorCode::InvalidInput);
    EXPECT_EQ(auth_.register_user("   ", "correct horse").error().code, ErrorCode::InvalidInput);
    EXPECT_EQ(auth_.register_user(std::string(31, 'a'), "correct horse").error().code,
              ErrorCode::InvalidInput);
    EXPECT_EQ(auth_.has_users().value(), false); // nothing was saved
}

TEST_F(AuthServiceTest, ATakenUsernameIsRejected) {
    ASSERT_TRUE(auth_.register_user("admin", "correct horse"));

    const auto again = auth_.register_user("admin", "another password");

    ASSERT_FALSE(again.has_value());
    EXPECT_EQ(again.error().code, ErrorCode::AlreadyExists);
}

// The real hasher: slow by design, so these use the "weak" setting to stay fast.
TEST(SodiumPasswordHasherTest, HashesAreArgon2idAndNotThePassword) {
    SodiumPasswordHasher hasher(HashStrength::WeakForTests);

    const auto hashed = hasher.hash("correct horse");

    ASSERT_TRUE(hashed.has_value());
    EXPECT_NE(*hashed, "correct horse");
    EXPECT_EQ(hashed->rfind("$argon2id$", 0), 0U); // starts with the algorithm name
}

TEST(SodiumPasswordHasherTest, VerifiesTheRightPasswordOnly) {
    SodiumPasswordHasher hasher(HashStrength::WeakForTests);
    const auto hashed = hasher.hash("correct horse").value();

    EXPECT_TRUE(hasher.verify(hashed, "correct horse"));
    EXPECT_FALSE(hasher.verify(hashed, "Correct horse"));
    EXPECT_FALSE(hasher.verify(hashed, ""));
    EXPECT_FALSE(hasher.verify("not a real hash", "correct horse"));
}

TEST(SodiumPasswordHasherTest, TheSamePasswordHashesDifferentlyEachTime) {
    SodiumPasswordHasher hasher(HashStrength::WeakForTests);

    EXPECT_NE(hasher.hash("correct horse").value(), hasher.hash("correct horse").value());
}

TEST(SodiumPasswordHasherTest, WorksEndToEndWithTheAuthService) {
    auto db = sqlite::Database::open(":memory:").value();
    ASSERT_TRUE(create_schema(db));
    SqliteUserRepository users(db);
    SodiumPasswordHasher hasher(HashStrength::WeakForTests);
    AuthService auth(users, hasher);

    ASSERT_TRUE(auth.register_user("admin", "correct horse"));

    EXPECT_TRUE(auth.log_in("admin", "correct horse").has_value());
    EXPECT_FALSE(auth.log_in("admin", "wrong horse").has_value());
}

} // namespace
} // namespace library
