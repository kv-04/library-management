# Library Management System

A console application for managing a school library: books, students, and who has borrowed
what. Written in modern C++20 on top of SQLite.

This is a ground-up rewrite of a library system I first wrote about three years ago as
single-file programs that stored raw object bytes in flat files. The rewrite focuses on clean
architecture, memory safety, automated tests, and professional tooling.

## Features

- Add, list, find, update, and delete **books** and **students**
- **Issue** a book to a student and **return** it, with a dated loan history
- A book can only be out once at a time; items with loan history can't be deleted
- **Login** with password hashing (Argon2id); the first run creates the administrator account
- **Backup** command that saves a complete copy of all data
- Friendly error messages ("book number 7 is already issued") and input validation
  everywhere: long, blank, or malformed input is rejected with a clear message

## Quick start

```sh
cmake --preset release
cmake --build --preset release
./build/release/src/library_app
```

On the first run the program asks you to create an account. Data is stored in `library.db`
in the current directory. To use a different file:

```sh
./build/release/src/library_app path/to/my-library.db
```

A walkthrough of the menus is in [docs/USER_GUIDE.md](docs/USER_GUIDE.md).

## Requirements

| Tool | Notes |
|---|---|
| C++20 compiler | Tested with GCC 13 on Linux. macOS (Apple clang) is a supported target |
| CMake 3.25+ | `brew install cmake` on macOS |
| SQLite 3 | Included with macOS. Debian/Ubuntu: `sudo apt install libsqlite3-dev` |
| Internet on first configure | CMake downloads and builds GoogleTest, `tl::expected`, and libsodium |

macOS and Linux only (the console uses POSIX terminal calls).

## Build, test, and quality checks

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug              # runs the unit tests
```

| Preset | What it does |
|---|---|
| `debug` | Debug build |
| `release` | Optimized build |
| `asan` | Debug build with AddressSanitizer and UndefinedBehaviorSanitizer |

Run the tests under the sanitizers (this is what CI does):

```sh
cmake --preset asan -DLIBRARY_WARNINGS_AS_ERRORS=ON
cmake --build --preset asan
ctest --preset asan
```

Format the code (CI fails on unformatted code):

```sh
find src tests \( -name '*.cpp' -o -name '*.hpp' \) | xargs clang-format -i
```

## How it is organized

```
src/
  domain/    Book, Student, errors, validation rules, repository interfaces. No I/O.
  storage/   SQLite wrappers, database schema, SQLite repositories.
  service/   Library rules (issue/return), login, password hashing.
  ui/        Console input/output and the menus.
  main.cpp   Wires everything together.
tests/       Unit tests, mirroring the src/ layout.
docs/        Architecture, user guide, ideas for future work.
```

Dependencies point one way only: `ui` -> `service` -> `domain` <- `storage`. See
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the reasoning behind each design choice.

## Highlights

- **Always-valid objects:** `Book` and `Student` can only be created through `create()`,
  which checks the rules, so invalid data can't exist in memory
- **Errors as values:** fallible operations return `Result<T>` (a value or an `Error`)
- **RAII everywhere:** database handles, statements, and terminal settings clean up after
  themselves; there is no manual `new`/`delete` memory management
- **Prepared statements only:** no SQL is ever built from user text, so SQL injection is not
  possible
- **Rules enforced by the database too:** unique keys, foreign keys, and a partial unique
  index that stops a book being issued twice
- **Testable UI:** the menus read from and write to any stream, so tests drive the whole
  program with a typed script
- **Quality gates:** strict compiler warnings, a sanitizer build, `clang-format`, and a
  GitHub Actions workflow for Linux and macOS

## Known limitations

- Single role: every logged-in user can do everything (role-based access is a listed
  next step)
- Text limits are in bytes, so accented characters count as more than one
- The GitHub Actions workflow and the `.clang-tidy` configuration are included but
  `clang-tidy` is not yet enforced in CI
- No due dates or fines yet

More ideas are in [docs/NEXT_LEVEL.md](docs/NEXT_LEVEL.md).

## License

MIT. See [LICENSE](LICENSE).
