_Pragma("once");

#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <tuple>
#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include "singleton.h"
#include "consts.h"


namespace leaf
{

class Tty final : public Singleton<Tty>
{
    friend Singleton<Tty>;
public:
    ~Tty();

    std::tuple<Key, std::string> getchar();

    // Moves the cursor n (default 1) cells in the given direction.
    // If the cursor is already at the edge of the screen, this has no effect.
    void moveCursor(CursorDirection dirction, uint32_t n=1) {
        switch ( dirction )
        {
        case CursorDirection::Up:
            fprintf(stdout_, "\033[%uA", n);
            break;
        case CursorDirection::Down:
            fprintf(stdout_, "\033[%uB", n);
            break;
        case CursorDirection::Right:
            fprintf(stdout_, "\033[%uC", n);
            break;
        case CursorDirection::Left:
            fprintf(stdout_, "\033[%uD", n);
            break;
        case CursorDirection::NextLine:
            // Moves cursor to beginning of the line n (default 1) lines down
            fprintf(stdout_, "\033[%uE", n);
            break;
        case CursorDirection::PrevLine:
            // Moves cursor to beginning of the line n (default 1) lines up
            fprintf(stdout_, "\033[%uF", n);
            break;
        case CursorDirection::HorizontalAbsolute:
            // Moves the cursor to column n (default 1)
            fprintf(stdout_, "\033[%uG", n);
            break;
        }
    }

    // Moves the cursor to line n, column m.
    // The values are 1-based, and default to 1 (top left corner) if omitted.
    void moveCursorTo(uint32_t line=1, uint32_t column=1) {
        fprintf(stdout_, "\033[%u;%uH", line, column);
    }

    void clear(EraseMode e) {
        switch ( e )
        {
        case EraseMode::ToScreenEnd:
            fprintf(stdout_, "\033[0J");
            break;
        case EraseMode::ToScreenBegin:
            fprintf(stdout_, "\033[1J");
            break;
        case EraseMode::EntireScreen:
            fprintf(stdout_, "\033[2J");
            break;
        case EraseMode::EntireScreenAndScroll:
            fprintf(stdout_, "\033[3J");
            break;
        case EraseMode::ToLineEnd:
            fprintf(stdout_, "\033[0K");
            break;
        case EraseMode::ToLineBegin:
            fprintf(stdout_, "\033[1K");
            break;
        case EraseMode::EntireLine:
            fprintf(stdout_, "\033[2K");
            break;
        }
    }

    // Scroll whole page up by n (default 1) lines.
    // New lines are added at the bottom.
    void scrollUp(uint32_t n=1) {
        fprintf(stdout_, "\033[%uS", n);
    }

    // Scroll whole page down by n (default 1) lines.
    // New lines are added at the top.
    void scrollDown(uint32_t n=1) {
        fprintf(stdout_, "\033[%uT", n);
    }

    void saveCursorPosition() {
        //fprintf(stdout_, "\033[s");
        fprintf(stdout_, "\0337");
    }

    void saveCursorPosition(uint32_t line, uint32_t col) {
        //fprintf(stdout_, "\033[%u;%uH\033[s", line, col);
        fprintf(stdout_, "\033[%u;%uH\0337", line, col);
    }

    void restoreCursorPosition() {
        //fprintf(stdout_, "\033[u");
        fprintf(stdout_, "\0338");
        fflush(stdout_);
    }

    void showCursor() {
        fprintf(stdout_, "\033[?25h");
        fflush(stdout_);
    }

    void showCursor_s() {
        write(term_stdout_, "\033[?25h", strlen("\033[?25h"));
    }

    void hideCursor() {
        fprintf(stdout_, "\033[?25l");
    }

    void enableAlternativeBuffer() {
        fprintf(stdout_, "\033[?1049h");
    }

    void disableAlternativeBuffer() {
        fprintf(stdout_, "\033[?1049l");
    }

    void disableAlternativeBuffer_s() {
        write(term_stdout_, "\033[?1049l", strlen("\033[?1049l"));
    }

    void enableMouse() {
        fprintf(stdout_, "\033[?1000;1006h");
    }

    void disableMouse() {
        fprintf(stdout_, "\033[?1000;1006l");
    }

    void disableMouse_s() {
        write(term_stdout_, "\033[?1000;1006l", strlen("\033[?1000;1006l"));
    }

    void enableAutoWrap() {
        fprintf(stdout_, "\033[?7h");
    }

    void enableAutoWrap_s() {
        write(term_stdout_, "\033[?7h", strlen("\033[?7h"));
    }

    void disableAutoWrap() {
        fprintf(stdout_, "\033[?7l");
    }

    // color is 0 - 255 or #000000 - #FFFFFF
    void setForegroundColor(const std::string& color) {
        if ( color[0] == '#' ) {
            fprintf(stdout_, "\033]10;%s\a", color.c_str());
        }
        else {
            auto index = stoi(color);
            if ( index >= 0 && index < 256 ) {
                fprintf(stdout_, "\033]10;%s\a", color_names_[index]);
            }
            else {
            }
        }
    }

    // color is 0 - 255 or #000000 - #FFFFFF
    void setBackgroundColor(const std::string& color) {
        if ( color[0] == '#' ) {
            fprintf(stdout_, "\033]11;%s\a", color.c_str());
        }
        else {
            auto index = stoi(color);
            if ( index >= 0 && index < 256 ) {
                fprintf(stdout_, "\033]11;%s\a", color_names_[index]);
            }
            else {
            }
        }
    }

    void resetForegroundColor() {
        fprintf(stdout_, "\033]110\a");
    }

    void resetBackgroundColor() {
        fprintf(stdout_, "\033]111\a");
    }

    void flush() {
        fflush(stdout_);
    }

    int getCursorPosition(uint32_t& line, uint32_t& col) {
        if ( write(term_stdout_, "\033[6n", 4) != 4 ) {
            perror("write");
            return -1;
        }

        char buffer[32] = { 0 };
        if ( read(term_stdin_, buffer, sizeof(buffer) - 1) < 0 ) {
            perror("read");
            return -1;
        }

        uint32_t i = 0;
        while ( buffer[i] != '\0' ) {
            if ( buffer[i] == '\033' ) {
                break;
            }
            ++i;
        }

        if ( sscanf(&buffer[i], "\033[%u;%uR", &line, &col) != 2 ) {
            return -1;
        }

        return 0;
    }

    int getCursorPosition2(uint32_t& line, uint32_t& col) {
        cursor_pos_ = true;
        if ( write(term_stdout_, "\033[6n", 4) != 4 ) {
            perror("write");
            return -1;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        while ( cursor_pos_ == true ) {
            cursor_cond_.wait(lock);
        }

        line = cursor_line_;
        col = cursor_col_;

        return 0;
    }

    int getWindowSize(uint32_t& lines, uint32_t& cols) {
        struct winsize ws;
        if (ioctl(term_stdout_, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
            saveCursorPosition();
            moveCursorTo(1024, 1024);
            fflush(stdout_);
            int ret = getCursorPosition(lines, cols);
            restoreCursorPosition();
            return ret;
        }
        else {
            cols = ws.ws_col;
            lines = ws.ws_row;
            return 0;
        }
    }

    int getWindowSize2(uint32_t& lines, uint32_t& cols) {
        struct winsize ws;
        if (ioctl(term_stdout_, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
            saveCursorPosition();
            moveCursorTo(1024, 1024);
            fflush(stdout_);
            int ret = getCursorPosition2(lines, cols);
            restoreCursorPosition();
            return ret;
        }
        else {
            cols = ws.ws_col;
            lines = ws.ws_row;
            return 0;
        }
    }

    struct String
    {
        template <typename T,
                  typename=std::enable_if_t<std::is_same<std::decay_t<T>, std::string>::value>
                 >
        static const char* c_str(T&& str) {
            return str.c_str();
        }

        template <typename T,
                  typename=std::enable_if_t<std::is_same<std::decay_t<T>,  const char*>::value
                      || std::is_same<std::decay_t<T>,  char*>::value>
                 >
        static const char* c_str(T str) {
            return str;
        }
    };

    template <typename T>
    void addString(T&& str) {
        fprintf(stdout_, "%s", String::c_str(std::forward<T>(str)));
    }

    template <typename T, typename C>
    void addString(T&& str, C&& color) {
        static_assert(std::is_same<std::decay_t<C>, std::string>::value, "color must be std::string!");
        fprintf(stdout_, "%s%s\033[0m", color.c_str(), String::c_str(std::forward<T>(str)));
    }

    template <typename T>
    void addString(uint32_t line, uint32_t col, T&& str) {
        fprintf(stdout_, "\033[%u;%uH%s", line, col, String::c_str(std::forward<T>(str)));
    }

    template <typename T, typename C>
    void addString(uint32_t line, uint32_t col, T&& str, C&& color) {
        static_assert(std::is_same<std::decay_t<C>, std::string>::value, "color must be std::string!");
        fprintf(stdout_, "\033[%u;%uH%s%s\033[0m", line, col, color.c_str(), String::c_str(std::forward<T>(str)));
    }

    template <typename T>
    void addStringAndSave(uint32_t line, uint32_t col, T&& str) {
        //fprintf(stdout_, "\033[%u;%uH%s\033[s", line, col, String::c_str(std::forward<T>(str)));
        fprintf(stdout_, "\033[%u;%uH%s\0337", line, col, String::c_str(std::forward<T>(str)));
    }

    template <typename T, typename C>
    void addStringAndSave(uint32_t line, uint32_t col, T&& str, C&& color) {
        static_assert(std::is_same<std::decay_t<C>, std::string>::value, "color must be std::string!");
        //fprintf(stdout_, "\033[%u;%uH%s%s\033[0m\033[s", line, col, color.c_str(), String::c_str(std::forward<T>(str)));
        fprintf(stdout_, "\033[%u;%uH%s%s\033[0m\0337", line, col, color.c_str(), String::c_str(std::forward<T>(str)));
    }

    void exit() {
        running_.store(false, std::memory_order_relaxed);
    }

    void restoreOrigTerminal();
    void restoreOrigTerminal_s();
    void setNewTerminal();
private:
    Tty();
    void _init();
    std::string _mouseTracking(const std::string& esc_code, Key& key) const;
    bool _getCursorPos(const std::string& esc_code);
private:
    int term_stdin_{ STDIN_FILENO };
    int term_stdout_{ STDOUT_FILENO };
    FILE* stdin_{ stdin };
    FILE* stdout_{ stdout };
    const char* color_names_[256];
    struct termios orig_term_;
    struct termios new_term_;
    using EscCodeMap = std::unordered_map<std::string, Key>;
    EscCodeMap esc_codes_;
    std::atomic<bool> running_{ true };
    bool cursor_pos_{ false };
    mutable std::mutex mutex_;
    std::condition_variable cursor_cond_;
    uint32_t cursor_line_{ 0 };
    uint32_t cursor_col_{ 0 };

};

} // end namespace leaf
