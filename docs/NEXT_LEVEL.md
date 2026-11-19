# Next-level features

Ideas for taking the project beyond a first release. Each one is chosen because it teaches
something employers look for. Difficulty is a rough guide (1 = an evening, 3 = a weekend or more).

| Idea | What I build | What it shows | Difficulty |
|---|---|---|---|
| **Due dates and fines** | Add a due date to loans; flag overdue books; compute a fine | Domain modeling, a schema change, date handling | 1 |
| **Better search** | Search books by part of the name or purpose; use SQLite's FTS5 full-text search for ranking | SQL, indexes, query design | 2 |
| **Role-based access** | Roles (admin, librarian, student) with permissions checked in the service layer | Authorization design, security thinking | 2 |
| **Schema migrations** | Track the database version with `PRAGMA user_version` and apply upgrade scripts in order | How real systems evolve data safely | 2 |
| **REST API** | Expose the service layer over HTTP (for example with cpp-httplib or Crow), returning JSON | Layered design paying off: the UI is replaced, the rules are untouched | 3 |
| **Dockerfile** | A multi-stage build that compiles in one image and runs in a small one | Deployment, reproducible builds | 1 |
| **Pagination and sorting** | List books in pages instead of all at once | Scaling thinking, API design | 1 |
| **CSV import / export** | Bulk-load books and students; export loans | File parsing, error reporting | 2 |
| **Configuration and CLI arguments** | Database path and options from a config file or flags (for example with CLI11) | Real-world program setup | 1 |
| **Structured logging** | Log operations and failures with spdlog | Observability | 1 |
| **Code coverage in CI** | Measure test coverage with gcov/lcov and publish it | Test quality metrics | 2 |
| **clang-tidy in CI** | Run the included `.clang-tidy` config on every pull request | Static analysis | 1 |
| **Fuzz testing** | Fuzz the input parsers with libFuzzer | Robustness against hostile input | 3 |
| **Package manager support** | Declare dependencies with vcpkg or Conan instead of FetchContent | Dependency management | 2 |
| **Release automation** | Tag a version and have CI publish binaries | Delivery pipelines | 2 |

## Suggested order

1. Due dates and fines (small, and uses the loan rules)
2. Dockerfile and clang-tidy in CI 
3. Role-based access
4. REST API 
