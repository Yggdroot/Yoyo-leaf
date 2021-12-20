#define TEST_RINGBUFFER

#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <memory>
#include "tty.h"
#include "color.h"

using namespace leaf;
using namespace std;

auto& tty = Tty::getInstance();

std::vector<string> source1 = {
    { "a" },
    { "ab" },
    { "a\r" },
    { "a\r\x7f" },
    { "a\rb" },
    { "a\033" },
    { "a\033b" },
    { "\033" },
    { "\033\033" },
    { "\033\r" },
    { "\r" },
    { "\r\n" },
    { "\r\n\n" },
    { "简单\r" },
    { "\033简单\r" },
    { "\033a" },
    { "\033[A" },
    { "\033[A\033[B" },
    { "\033[A\033[B\033" },
};

std::vector<std::vector<char>> source2 = {
    { '\0' },
    { '\0', '\0' },
    { '\0', '\0', '\0' },
    { '\0', '\r' },
    { 'a', '\0' },
    { 'a', '\0', 'a' },
    { 'q' },
};

const char* tmpfile_name = "/tmp/test_getchar";

void getchar_print() {
    while ( true ) {
        auto tu = tty.getchar();
        auto key = std::get<0>(tu);
        if ( key == Key::Timeout ) {
            printf("timeout\r\n");
            if ( std::get<1>(tu).empty() ) {
                break;
            }
        }
        else {
            printf("%-5d", int(key));
            auto str = std::get<1>(tu);
            if ( !str.empty() ) {
                if (str[0] == '\033')
                    printf("\\033%s", str.substr(1).c_str());
                else {
                    printf("%s", str.c_str());
                }
            }
            printf("\r\n");
        }
    }
}

void test_tty() {
    cout << "Makefile" << "\r\n";
    cout << "src/utils.cpp" << "\r\n";
    cout << "src/tty.h" << "\r\n";
    cout << "src/consts.h" << "\r\n";
    cout << "src/app.cpp" << "\r\n";
    cout << "src/utils.h" << "\r\n";
    cout << "src/tui.h" << "\r\n";
    cout << "src/main.cpp" << "\r\n";
    cout << "src/tty.cpp" << "\r\n";
    cout << "src/Makefile" << "\r\n";
    cout << "src/tui.cpp" << "\r\n";
    cout << "src/color.h" << "\r\n";
    cout << "README.md" << "\r\n";
    cout << "LICENSE" << "\r\n";

    while ( true ) {
        auto tu = tty.getchar();
        auto ch = std::get<1>(tu);
        if ( ch == "q" ) {
            break;
        }
        switch ( ch[0] )
        {
        case 'h':
            tty.moveCursor(CursorDirection::Left);
            break;
        case 'j':
            tty.moveCursor(CursorDirection::Down);
            break;
        case 'k':
            tty.moveCursor(CursorDirection::Up);
            break;
        case 'l':
            tty.moveCursor(CursorDirection::Right);
            break;
        case 'n':
            tty.moveCursor(CursorDirection::NextLine);
            break;
        case 'p':
            tty.moveCursor(CursorDirection::PrevLine);
            break;
        case 'u':
            tty.clear(EraseMode::ToScreenBegin);
            break;
        case 'd':
            tty.clear(EraseMode::ToScreenEnd);
            break;
        case 'D':
            tty.clear(EraseMode::EntireScreen);
            break;
        case '0':
            tty.clear(EraseMode::ToLineBegin);
            break;
        case '1':
            tty.clear(EraseMode::ToLineEnd);
            break;
        case 'L':
            tty.clear(EraseMode::EntireLine);
            break;
        case 'c':
            tty.hideCursor();
            break;
        case 'C':
            tty.showCursor();
            break;
        case '[':
            tty.setForegroundColor("137");
            break;
        case ']':
            tty.setBackgroundColor("235");
            break;
        case '{':
            tty.resetForegroundColor();
            break;
        case '}':
            tty.resetBackgroundColor();
            break;
        case 'a':
            tty.enableAlternativeBuffer();
            break;
        case 'b':
            tty.disableAlternativeBuffer();
            break;
        case 's':
            tty.scrollUp();
            break;
        case 't':
            tty.scrollDown();
            break;
        case '<':
            tty.saveCursorPosition();
            break;
        case '>':
            tty.restoreCursorPosition();
            break;
        case ';':
            uint32_t x, y;
            tty.getCursorPosition(x, y);
            printf("%u, %u\n", x, y);
            break;
        case '/':
            tty.saveCursorPosition();
            tty.addString(3,1,"disableAlternativeBuffer\r\n", Color(Color256("121", "")).getColor());
            tty.restoreCursorPosition();
            break;
        case 'm':
            uint32_t r, c;
            tty.getWindowSize(r, c);
            printf("%u, %u\n", r, c);
            break;
        }

        tty.flush();
    }
}

void test_getchar() {
    auto destroy = [](FILE* f) { fclose(f); };
    std::unique_ptr<FILE, decltype(destroy)> file(fopen(tmpfile_name, "w"), destroy);
    if ( file == nullptr ) {
        cout << "fopen failed!" << endl;
        return;
    }

    if ( freopen(tmpfile_name, "r", stdin) == nullptr ) {
        cout << "freopen failed!" << endl;
        return;
    }

    auto f = file.get();
    for ( auto& s: source1 ) {
        cout << "------------------------------------------------------\r\n";
        fwrite(s.c_str(), s.length(), 1, f);
        fflush(f);
        getchar_print();
    }

    for ( auto& v: source2 ) {
        cout << "------------------------------------------------------\r\n";
        fwrite(v.data(), v.size(), 1, f);
        fflush(f);
        getchar_print();
    }

    (void)freopen("/dev/tty", "r", stdin);

}

int main(int argc, const char *argv[])
{
    test_tty();
    test_getchar();

    return 0;
}

