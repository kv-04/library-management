#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

namespace library {

// Everything the program needs to talk to a person: print lines, ask questions, read answers.
// It works on any input/output stream, so tests can feed it a script instead of a keyboard.
//
// Every read returns std::nullopt when the input ENDS (Ctrl-D, closed pipe). Callers stop
// instead of asking forever, which fixes the old programs' endless-loop problem.
class Console {
  public:
    Console(std::istream& in, std::ostream& out, bool hide_password_echo = false);

    [[nodiscard]] std::ostream& out() { return out_; }

    void println(std::string_view text = {});

    // Shows the prompt and returns one line (without the line break).
    [[nodiscard]] std::optional<std::string> read_line(std::string_view prompt);

    // Keeps asking until the answer is a whole number.
    [[nodiscard]] std::optional<int> read_int(std::string_view prompt);

    // Like read_line, but the typed characters are not shown on screen when
    // hide_password_echo is on and the input is a real terminal.
    [[nodiscard]] std::optional<std::string> read_password(std::string_view prompt);

    // Asks a yes/no question. Anything that does not start with y or Y counts as no.
    [[nodiscard]] bool confirm(std::string_view question);

  private:
    std::istream& in_;
    std::ostream& out_;
    bool hide_password_echo_;
};

} // namespace library
