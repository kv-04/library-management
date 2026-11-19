# Architecture

How the code is organized and why.

## The rule that shapes everything

**Dependencies point one way.** Each layer may use the layers below it and must never reach
upward.

```
  ┌──────────────────────────────┐
  │  ui/       console menus     │  asks questions, prints answers
  └───────────────┬──────────────┘
                  ▼
  ┌──────────────────────────────┐
  │  service/  the library rules │  issue, return, log in
  └───────┬───────────────┬──────┘
          ▼               ▼
  ┌──────────────┐   ┌──────────────────────────┐
  │  domain/     │◄──┤  storage/                │
  │  Book,       │   │  SQLite wrappers and     │
  │  Student,    │   │  repositories            │
  │  Error,      │   │                          │
  │  interfaces  │   │                          │
  └──────────────┘   └──────────────────────────┘
```

Why this matters: the rules can be tested without a screen, the screens can be tested
without a person, the database could be swapped without touching the rules, and the console
could be replaced by a web API without touching either.

`main.cpp` is the only place that knows about every layer. It creates the pieces and plugs
them together ("dependency injection by hand").

## Modules

| Module | Location | Responsibility |
|---|---|---|
| Errors | `domain/error.*` | `ErrorCode`, `Error`, `Result<T>` |
| Text rules | `domain/text.*` | `trim`, `clean_text` (trim, non-empty, max length) |
| Book, Student | `domain/book.*`, `domain/student.*` | Validated entities |
| Repository interfaces | `domain/repositories.hpp` | What can be saved/loaded, not how |
| SQLite wrapper | `storage/sqlite.*` | RAII `Database` and `Statement`, error translation |
| Schema | `storage/schema.*` | Table definitions |
| SQLite repositories | `storage/sqlite_repositories.*` | The interfaces, implemented with SQL |
| Library service | `service/library_service.*` | Books, students, issue/return, backup |
| Auth service | `service/auth_service.*` | Registration and login |
| Password hasher | `service/password_hasher.hpp`, `sodium_password_hasher.*` | Interface and Argon2id version |
| Console | `ui/console.*` | Reading and printing, safe number input |
| Console app | `ui/console_app.*` | The menus |

## Key design decisions

### Validated domain objects (private constructor + `create()`)
`Book` and `Student` can only be built by `create()`, which returns a `Result`. Because the
constructor is private, an invalid object cannot exist, and no other code re-checks the
rules. The old code accepted anything.

### `Result<T>` instead of exceptions or return codes
Operations that can fail return either the value or an `Error` (a code plus a message). The
failure is part of the function signature and cannot be ignored by accident
(`[[nodiscard]]`). Codes (`NotFound`, `AlreadyExists`, `Conflict`, `InvalidInput`,
`Unauthorized`, `StorageFailure`) let callers react without parsing text.

`Result` is an alias for `tl::expected`, which has the same interface as C++23's
`std::expected` but works with C++20 toolchains. The project targets C++20 because the
development machine's Apple Command Line Tools shipped an incomplete C++23 library. Moving to
`std::expected` later means editing only the alias and `make_error()` in `error.hpp`.

Exceptions are used only where nothing sensible can be done: if libsodium cannot start,
the password hasher's constructor throws and `main` reports it.

### Repository interfaces in the domain layer
`BookRepository`, `StudentRepository`, `IssueRepository`, `UserRepository`, and `BackupStore`
are abstract classes in `domain/`. The SQLite versions live in `storage/`. The domain
defines what it needs; storage provides it. `find()` returns
`Result<std::optional<T>>`: "not found" is a normal answer (the optional), while a database
failure is an error (the Result).

### Services translate errors for people
Repositories report technical problems (`UNIQUE constraint failed`). `LibraryService` turns
them into messages a user understands ("book number 7 already exists") and adds the rules
that span several tables (issuing checks the book, then the student, then the loan).

### RAII wrappers
`sqlite::Database` and `sqlite::Statement` own their SQLite handles through
`std::unique_ptr` with custom deleters. They are movable, not copyable. Handles are released
on every path, including errors. The database closes with `sqlite3_close_v2`, so a statement
that outlives its `Database` is safe. The console's `EchoGuard` switches terminal echo off
for password entry and restores it automatically.

### Prepared statements only
Values reach SQL through `bind()` and `?` placeholders (`Database::run` and `query` take the
values as arguments). There is no string concatenation into SQL, which rules out SQL
injection. A test stores `Robert'); DROP TABLE t;--` as data and verifies the table survives.

### The database enforces the important rules too
- `PRIMARY KEY` on book numbers, roll numbers, and usernames
- Foreign keys on loans (switched on explicitly, since SQLite ignores them by default)
- A partial unique index (`one_open_issue_per_book`) means a book can only have one loan
  without a return date, so even a bug in the service layer could not lend a book twice

### Passwords
Only hashes are stored, using libsodium's Argon2id (`crypto_pwhash_str`), with a random salt
per password. The `PasswordHasher` interface lets login rules be tested instantly with a
fake hasher; the real hasher has its own tests at a deliberately weak setting. A wrong
password and an unknown username return the same error, so the login prompt can't be used to
discover usernames. libsodium is fetched and built by CMake, so users install nothing.

### Input handling
`Console` reads whole lines and parses numbers with `std::from_chars`. Bad input is rejected
and asked again; end of input returns `std::nullopt` and every menu unwinds cleanly. This
fixes the original program's infinite loop on non-numeric input.

## Data model

| Table | Columns | Enforced by the database |
|---|---|---|
| `books` | `book_no` (key), `name`, `purpose` | unique number |
| `students` | `roll_no` (key), `name`, `school` | unique number |
| `issues` | `id`, `book_no`, `roll_no`, `issued_on`, `returned_on` | references a real book and student; one open loan per book |
| `users` | `username` (key), `password_hash` | unique username |

Loans store the student's roll number only; the name is joined from `students`, so it can't
drift out of sync. Dates are stored as `YYYY-MM-DD` text.

## Testing strategy

| Layer | How it is tested |
|---|---|
| Domain | Every validation rule, including exactly-at and one-over each limit |
| Storage | Against `:memory:` databases: fast, isolated, no files left behind |
| Services | Against in-memory databases, with a fixed "today" and a fake password hasher |
| UI | The whole menu system is driven with a typed script and its output is checked |

Every test also runs under AddressSanitizer and UndefinedBehaviorSanitizer in CI. Passwords
use a fast fake in most tests; the real hasher is tested at a weak setting so the suite
stays quick.

## Tooling and CI

- **CMake presets:** `debug`, `release`, `asan`
- **Warnings:** `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
  -Wold-style-cast` and more; `-Werror` in CI
- **Formatting:** `clang-format`, checked in CI
- **GitHub Actions:** build and test on Ubuntu (GCC and Clang) and macOS, a sanitizer job,
  and a format check
- **Linting:** `.clang-tidy` is configured but not yet enforced in CI

## What changed from the original program

| Original | Now |
|---|---|
| `char name[50]` + `strcpy` | `std::string`, with enforced length limits |
| Raw object bytes written to `.dat` files | SQLite database |
| Return codes `0`/`1` | `std::optional` and `Result<T>` |
| Manual `fin.close()` (sometimes unreachable) | RAII |
| Functions and `friend`s mixing menus, rules, and files | Four separate layers |
| Three copies of each module | One definition per module |
| Login accepted anything | Argon2id-hashed accounts |
| No way to return a book | Issue and return with history |
| Menu froze on non-numbers | Re-asks; handles end of input |
| "Copy" could corrupt `book.dat` | Backup to a new, dated file |
| No tests, no build system | Unit tests, CMake, CI |

### Behavior changes, and why
1. Book and roll numbers must be positive and unique everywhere (the old "write" and
   "insert" menus disagreed).
2. Names with spaces work (`getline` instead of `cin >>`).
3. Issuing uses the validated book and student instead of re-asking.
4. A book that is out can't be issued again; items with loan history can't be deleted.
5. Delete asks for confirmation; update keeps fields left blank.
6. No screen clearing or "enter 1 to go back" pauses: the menus simply loop.
7. Per-entity "copy" menu items became one backup command in the main menu.
8. Real login, with first-run account creation.
