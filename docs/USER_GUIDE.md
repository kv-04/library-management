# User guide

## Starting the program

```sh
./build/release/src/library_app            # uses library.db in the current folder
./build/release/src/library_app my.db      # uses a different file
./build/release/src/library_app --help
```

## First run: creating your account

If the database has no accounts yet, the program asks you to create one:

```
First run: let's create the administrator account.
Choose a username:
Choose a password (8+ characters):
Repeat the password:
```

Passwords are hidden while you type. After that, the program logs you in. On later runs you
are asked for your username and password, with three attempts.

## The menus

Type the number of an option and press Enter. `0` always goes back (or exits, from the
main menu). If you type something that isn't a number, the program just asks again.

```
MAIN MENU
1. Books
2. Students
3. Issue / return books
4. Back up the data
0. Log out and exit
```

### Books

| Option | What it does |
|---|---|
| Add a book | Asks for a book number, name, and purpose |
| List all books | Shows a table of every book, in number order |
| Find a book | Looks up one book by number |
| Update a book | Press Enter on a question to keep the current value |
| Delete a book | Asks "Delete book 7 'Dune'? (y/N)" before deleting |

### Students

Same options as books, using the roll number, name, and school.

### Issue / return

| Option | What it does |
|---|---|
| Issue a book | Asks for the book number and the student's roll number. Records today's date |
| Return a book | Asks for the book number. Records the return date |
| Loan history | Shows every loan, newest first, with "not yet" for books still out |

### Back up the data

Saves a full copy of the database to a new file in the current folder, named like
`library-backup-20261113-193000.db`. A backup never overwrites an existing file. To restore
one, start the program with that file: `./library_app library-backup-20261113-193000.db`.

## Rules the program enforces

- Book numbers and roll numbers must be positive and unique
- Names and purposes can't be empty; books allow up to 50 characters, students' names and
  schools up to 20
- A book can only be issued if it exists, the student exists, and it isn't already out
- A book or student with loan history can't be deleted

## Common messages

| Message | Meaning |
|---|---|
| `book number 7 already exists` | That number is taken; pick another |
| `book number 7 is already issued` | Return it first |
| `book number 7 is not currently issued` | Nothing to return |
| `book number 7 has loan history and cannot be deleted` | It was lent out before |
| `wrong username or password` | Check both; the same message is shown for either mistake |
