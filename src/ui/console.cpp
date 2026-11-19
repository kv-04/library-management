#include "ui/console.hpp"

#include "domain/text.hpp"

#include <charconv>
#include <iostream>
#include <termios.h>
#include <unistd.h>

namespace library {

namespace {

// Turns off typed-character echo for as long as the object exists, then puts the terminal
// back exactly as it was, even if something goes wrong in between (RAII again).
class EchoGuard {
  public:
    EchoGuard() {
        if (isatty(STDIN_FILENO) != 0 && tcgetattr(STDIN_FILENO, &saved_) == 0) {
            termios silent = saved_;
            silent.c_lflag &= ~static_cast<tcflag_t>(ECHO);
            active_ = tcsetattr(STDIN_FILENO, TCSANOW, &silent) == 0;
        }
    }
    ~EchoGuard() {
        if (active_) {
            tcsetattr(STDIN_FILENO, TCSANOW, &saved_);
        }
    }
    EchoGuard(const EchoGuard&) = delete;
    EchoGuard& operator=(const EchoGuard&) = delete;

  private:
    termios saved_{};
    bool active_ = false;
};

void drop_carriage_return(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

} // namespace

Console::Console(std::istream& in, std::ostream& out, bool hide_password_echo)
    : in_(in), out_(out), hide_password_echo_(hide_password_echo) {}

void Console::println(std::string_view text) {
    out_ << text << '\n';
}

std::optional<std::string> Console::read_line(std::string_view prompt) {
    out_ << prompt << std::flush;
    std::string line;
    if (!std::getline(in_, line)) {
        return std::nullopt;
    }
    drop_carriage_return(line);
    return line;
}

std::optional<int> Console::read_int(std::string_view prompt) {
    while (true) {
        const auto line = read_line(prompt);
        if (!line) {
            return std::nullopt;
        }
        const std::string text = trim(*line);
        int value = 0;
        const char* const begin = text.data();
        const char* const end = begin + text.size();
        const auto [stopped_at, error] = std::from_chars(begin, end, value);
        if (!text.empty() && error == std::errc{} && stopped_at == end) {
            return value;
        }
        println("Please enter a whole number.");
    }
}

std::optional<std::string> Console::read_password(std::string_view prompt) {
    if (!hide_password_echo_) {
        return read_line(prompt);
    }
    out_ << prompt << std::flush;
    std::string line;
    bool got_line = false;
    {
        const EchoGuard guard;
        got_line = static_cast<bool>(std::getline(in_, line));
    }
    out_ << '\n'; // the Enter key was not echoed, so end the line ourselves
    if (!got_line) {
        return std::nullopt;
    }
    drop_carriage_return(line);
    return line;
}

bool Console::confirm(std::string_view question) {
    const auto answer = read_line(question);
    if (!answer) {
        return false;
    }
    const std::string text = trim(*answer);
    return !text.empty() && (text[0] == 'y' || text[0] == 'Y');
}

} // namespace library
