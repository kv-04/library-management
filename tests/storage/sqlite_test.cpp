#include "storage/sqlite.hpp"

#include <gtest/gtest.h>
#include <string>
#include <utility>

namespace library::sqlite {
namespace {

// A throwaway database that lives in memory. .value() throws if opening fails, which
// GoogleTest reports as a failed test.
Database open_memory() {
    return Database::open(":memory:").value();
}

TEST(SqliteTest, OpensAnInMemoryDatabase) {
    EXPECT_TRUE(Database::open(":memory:").has_value());
}

TEST(SqliteTest, OpeningAnUnreachablePathFailsWithAReadableError) {
    const auto db = Database::open("/this/folder/does/not/exist/library.db");

    ASSERT_FALSE(db.has_value());
    EXPECT_EQ(db.error().code, ErrorCode::StorageFailure);
    EXPECT_NE(db.error().message.find("library.db"), std::string::npos);
}

TEST(SqliteTest, ExecuteRunsValidSql) {
    auto db = open_memory();

    EXPECT_TRUE(db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)").has_value());
    EXPECT_TRUE(db.execute("INSERT INTO t VALUES (1, 'Dune')").has_value());
}

TEST(SqliteTest, ExecuteReportsBrokenSql) {
    auto db = open_memory();

    const auto result = db.execute("CREATE TABEL t (id INTEGER)");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::StorageFailure);
    EXPECT_NE(result.error().message.find("syntax"), std::string::npos);
}

TEST(SqliteTest, PrepareReportsBrokenSql) {
    auto db = open_memory();

    const auto statement = db.prepare("SELECT * FROM missing_table");

    ASSERT_FALSE(statement.has_value());
    EXPECT_NE(statement.error().message.find("missing_table"), std::string::npos);
}

TEST(SqliteTest, StoresAndReadsBackValuesThroughBoundParameters) {
    auto db = open_memory();
    ASSERT_TRUE(db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)"));

    auto insert = db.prepare("INSERT INTO t (id, name) VALUES (?, ?)").value();
    ASSERT_TRUE(insert.bind(1, 7));
    ASSERT_TRUE(insert.bind(2, "The Hobbit"));
    EXPECT_EQ(insert.step().value(), false); // an INSERT produces no rows

    auto select = db.prepare("SELECT id, name FROM t").value();
    ASSERT_EQ(select.step().value(), true);
    EXPECT_EQ(select.column_int(0), 7);
    EXPECT_EQ(select.column_text(1), "The Hobbit");
    EXPECT_EQ(select.step().value(), false); // no more rows
}

TEST(SqliteTest, BoundTextIsNeverTreatedAsSql) {
    auto db = open_memory();
    ASSERT_TRUE(db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)"));
    const std::string nasty = "Robert'); DROP TABLE t;--";

    auto insert = db.prepare("INSERT INTO t (id, name) VALUES (1, ?)").value();
    ASSERT_TRUE(insert.bind(1, nasty));
    ASSERT_TRUE(insert.step());

    auto select = db.prepare("SELECT name FROM t").value(); // table still exists
    ASSERT_EQ(select.step().value(), true);
    EXPECT_EQ(select.column_text(0), nasty);
}

TEST(SqliteTest, AStatementCanBeResetAndReused) {
    auto db = open_memory();
    ASSERT_TRUE(db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY)"));
    auto insert = db.prepare("INSERT INTO t (id) VALUES (?)").value();

    for (int id = 1; id <= 3; ++id) {
        ASSERT_TRUE(insert.bind(1, id));
        ASSERT_TRUE(insert.step());
        ASSERT_TRUE(insert.reset());
    }

    auto count = db.prepare("SELECT COUNT(*) FROM t").value();
    ASSERT_TRUE(count.step().value());
    EXPECT_EQ(count.column_int(0), 3);
}

TEST(SqliteTest, DistinguishesNullFromEmptyText) {
    auto db = open_memory();
    ASSERT_TRUE(db.execute("CREATE TABLE t (a TEXT, b TEXT)"));
    auto insert = db.prepare("INSERT INTO t (a, b) VALUES (?, ?)").value();
    ASSERT_TRUE(insert.bind_null(1));
    ASSERT_TRUE(insert.bind(2, ""));
    ASSERT_TRUE(insert.step());

    auto select = db.prepare("SELECT a, b FROM t").value();
    ASSERT_TRUE(select.step().value());
    EXPECT_TRUE(select.column_is_null(0));
    EXPECT_FALSE(select.column_is_null(1));
    EXPECT_EQ(select.column_text(1), "");
}

TEST(SqliteTest, DuplicatePrimaryKeyIsReportedAsAlreadyExists) {
    auto db = open_memory();
    ASSERT_TRUE(db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY)"));
    ASSERT_TRUE(db.execute("INSERT INTO t VALUES (1)"));

    const auto duplicate = db.execute("INSERT INTO t VALUES (1)");

    ASSERT_FALSE(duplicate.has_value());
    EXPECT_EQ(duplicate.error().code, ErrorCode::AlreadyExists);
}

TEST(SqliteTest, ForeignKeysAreEnforced) {
    auto db = open_memory();
    ASSERT_TRUE(db.execute("CREATE TABLE parent (id INTEGER PRIMARY KEY)"));
    ASSERT_TRUE(db.execute(
        "CREATE TABLE child (id INTEGER PRIMARY KEY, parent_id INTEGER REFERENCES parent(id))"));

    const auto orphan = db.execute("INSERT INTO child VALUES (1, 99)");

    ASSERT_FALSE(orphan.has_value());
    EXPECT_EQ(orphan.error().code, ErrorCode::Conflict);
}

TEST(SqliteTest, ChangesCountsTheRowsTheLastCommandTouched) {
    auto db = open_memory();
    ASSERT_TRUE(db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)"));
    ASSERT_TRUE(db.execute("INSERT INTO t VALUES (1, 'a'), (2, 'b')"));

    ASSERT_TRUE(db.execute("UPDATE t SET name = 'z'"));
    EXPECT_EQ(db.changes(), 2);

    ASSERT_TRUE(db.execute("DELETE FROM t WHERE id = 99"));
    EXPECT_EQ(db.changes(), 0);
}

TEST(SqliteTest, ADatabaseCanBeMovedAndStillWorks) {
    auto original = open_memory();
    Database moved = std::move(original);

    EXPECT_TRUE(moved.execute("CREATE TABLE t (id INTEGER)").has_value());
}

TEST(SqliteTest, AStatementMayOutliveItsDatabaseObjectSafely) {
    auto statement = [] {
        auto db = open_memory();
        EXPECT_TRUE(db.execute("CREATE TABLE t (id INTEGER)"));
        return db.prepare("SELECT 1").value();
    }(); // the Database is destroyed here, the Statement lives on

    ASSERT_TRUE(statement.step().value());
    EXPECT_EQ(statement.column_int(0), 1);
}

TEST(SqliteTest, QueryBindsAllArgumentsInOrder) {
    auto db = open_memory();
    ASSERT_TRUE(db.execute("CREATE TABLE t (a INTEGER, b TEXT)"));
    ASSERT_TRUE(db.run("INSERT INTO t (a, b) VALUES (?, ?)", 5, "five"));
    ASSERT_TRUE(db.run("INSERT INTO t (a, b) VALUES (?, ?)", 6, std::string("six")));

    auto rows = db.query("SELECT b FROM t WHERE a = ?", 6).value();

    ASSERT_TRUE(rows.step().value());
    EXPECT_EQ(rows.column_text(0), "six");
}

TEST(SqliteTest, RunWorksWithNoArgumentsAndReportsErrors) {
    auto db = open_memory();

    EXPECT_TRUE(db.run("CREATE TABLE t (id INTEGER)").has_value());
    EXPECT_FALSE(db.run("INSERT INTO nowhere VALUES (1)").has_value());
}

} // namespace
} // namespace library::sqlite
