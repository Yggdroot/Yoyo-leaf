#include <cstdlib>
#include <cctype>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex>
#include <chrono>
#include "tty.h"
#include "utils.h"
#include "error.h"

namespace leaf
{

Tty::Tty(): color_names_ {
    "Black",           "Maroon",            "Green",           "Olive",
    "Navy",            "Purple",            "Teal",            "Silver",
    "Grey",            "Red",               "Lime",            "Yellow",
    "Blue",            "Fuchsia",           "Aqua",            "White",
    "Grey0",           "NavyBlue",          "DarkBlue",        "Blue3",
    "Blue3",           "Blue1",             "DarkGreen",       "DeepSkyBlue4",
    "DeepSkyBlue4",    "DeepSkyBlue4",      "DodgerBlue3",     "DodgerBlue2",
    "Green4",          "SpringGreen4",      "Turquoise4",      "DeepSkyBlue3",
    "DeepSkyBlue3",    "DodgerBlue1",       "Green3",          "SpringGreen3",
    "DarkCyan",        "LightSeaGreen",     "DeepSkyBlue2",    "DeepSkyBlue1",
    "Green3",          "SpringGreen3",      "SpringGreen2",    "Cyan3",
    "DarkTurquoise",   "Turquoise2",        "Green1",          "SpringGreen2",
    "SpringGreen1",    "MediumSpringGreen", "Cyan2",           "Cyan1",
    "DarkRed",         "DeepPink4",         "Purple4",         "Purple4",
    "Purple3",         "BlueViolet",        "Orange4",         "Grey37",
    "MediumPurple4",   "SlateBlue3",        "SlateBlue3",      "RoyalBlue1",
    "Chartreuse4",     "DarkSeaGreen4",     "PaleTurquoise4",  "SteelBlue",
    "SteelBlue3",      "CornflowerBlue",    "Chartreuse3",     "DarkSeaGreen4",
    "CadetBlue",       "CadetBlue",         "SkyBlue3",        "SteelBlue1",
    "Chartreuse3",     "PaleGreen3",        "SeaGreen3",       "Aquamarine3",
    "MediumTurquoise", "SteelBlue1",        "Chartreuse2",     "SeaGreen2",
    "SeaGreen1",       "SeaGreen1",         "Aquamarine1",     "DarkSlateGray2",
    "DarkRed",         "DeepPink4",         "DarkMagenta",     "DarkMagenta",
    "DarkViolet",      "Purple",            "Orange4",         "LightPink4",
    "Plum4",           "MediumPurple3",     "MediumPurple3",   "SlateBlue1",
    "Yellow4",         "Wheat4",            "Grey53",          "LightSlateGrey",
    "MediumPurple",    "LightSlateBlue",    "Yellow4",         "DarkOliveGreen3",
    "DarkSeaGreen",    "LightSkyBlue3",     "LightSkyBlue3",   "SkyBlue2",
    "Chartreuse2",     "DarkOliveGreen3",   "PaleGreen3",      "DarkSeaGreen3",
    "DarkSlateGray3",  "SkyBlue1",          "Chartreuse1",     "LightGreen",
    "LightGreen",      "PaleGreen1",        "Aquamarine1",     "DarkSlateGray1",
    "Red3",            "DeepPink4",         "MediumVioletRed", "Magenta3",
    "DarkViolet",      "Purple",            "DarkOrange3",     "IndianRed",
    "HotPink3",        "MediumOrchid3",     "MediumOrchid",    "MediumPurple2",
    "DarkGoldenrod",   "LightSalmon3",      "RosyBrown",       "Grey63",
    "MediumPurple2",   "MediumPurple1",     "Gold3",           "DarkKhaki",
    "NavajoWhite3",    "Grey69",            "LightSteelBlue3", "LightSteelBlue",
    "Yellow3",         "DarkOliveGreen3",   "DarkSeaGreen3",   "DarkSeaGreen2",
    "LightCyan3",      "LightSkyBlue1",     "GreenYellow",     "DarkOliveGreen2",
    "PaleGreen1",      "DarkSeaGreen2",     "DarkSeaGreen1",   "PaleTurquoise1",
    "Red3",            "DeepPink3",         "DeepPink3",       "Magenta3",
    "Magenta3",        "Magenta2",          "DarkOrange3",     "IndianRed",
    "HotPink3",        "HotPink2",          "Orchid",          "MediumOrchid1",
    "Orange3",         "LightSalmon3",      "LightPink3",      "Pink3",
    "Plum3",           "Violet",            "Gold3",           "LightGoldenrod3",
    "Tan",             "MistyRose3",        "Thistle3",        "Plum2",
    "Yellow3",         "Khaki3",            "LightGoldenrod2", "LightYellow3",
    "Grey84",          "LightSteelBlue1",   "Yellow2",         "DarkOliveGreen1",
    "DarkOliveGreen1", "DarkSeaGreen1",     "Honeydew2",       "LightCyan1",
    "Red1",            "DeepPink2",         "DeepPink1",       "DeepPink1",
    "Magenta2",        "Magenta1",          "OrangeRed1",      "IndianRed1",
    "IndianRed1",      "HotPink",           "HotPink",         "MediumOrchid1",
    "DarkOrange",      "Salmon1",           "LightCoral",      "PaleVioletRed1",
    "Orchid2",         "Orchid1",           "Orange1",         "SandyBrown",
    "LightSalmon1",    "LightPink1",        "Pink1",           "Plum1",
    "Gold1",           "LightGoldenrod2",   "LightGoldenrod2", "NavajoWhite1",
    "MistyRose1",      "Thistle1",          "Yellow1",         "LightGoldenrod1",
    "Khaki1",          "Wheat1",            "Cornsilk1",       "Grey100",
    "Grey3",           "Grey7",             "Grey11",          "Grey15",
    "Grey19",          "Grey23",            "Grey27",          "Grey30",
    "Grey35",          "Grey39",            "Grey42",          "Grey46",
    "Grey50",          "Grey54",            "Grey58",          "Grey62",
    "Grey66",          "Grey70",            "Grey74",          "Grey78",
    "Grey82",          "Grey85",            "Grey89",          "Grey93"
}
{
    _init();
}

Tty::~Tty() {
    fclose(stdin_);
    fclose(stdout_);
}

void Tty::_init() {
    stdin_ = fopen("/dev/tty", "r");
    if ( stdin_ == nullptr ) {
        Error::getInstance().appendError(utils::strFormat("%s:%d:%s", __FILE__, __LINE__, strerror(errno)));
        std::exit(EXIT_FAILURE);
    }
    term_stdin_ = fileno(stdin_);

    stdout_ = fopen("/dev/tty", "w");
    if ( stdout_ == nullptr ) {
        Error::getInstance().appendError(utils::strFormat("%s:%d:%s", __FILE__, __LINE__, strerror(errno)));
        std::exit(EXIT_FAILURE);
    }
    term_stdout_ = fileno(stdout_);

    if ( tcgetattr(term_stdin_, &orig_term_) == -1 ) {
        Error::getInstance().appendError(utils::strFormat("%s:%d:%s", __FILE__, __LINE__, strerror(errno)));
        std::exit(EXIT_FAILURE);
    }

    new_term_ = orig_term_;
    // ICRNL separate Ctrl-M from Ctrl-J
    // IXON disable Ctrl-S and Ctrl-Q
    new_term_.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    new_term_.c_oflag &= ~(OPOST);
    // IEXTEN disable Ctrl-V
    new_term_.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    new_term_.c_cflag |= (CS8);
    new_term_.c_cc[VMIN] = 0;
    new_term_.c_cc[VTIME] = 1;

    //new_term_.c_iflag |= (IUTF8);

    setNewTerminal();

#if 0
    // shell command
    for i in `echo "vt100 vt102 vt220 vt320 vt400 \
       xterm xterm-r5 xterm-r6 xterm-new \
       xterm-16color xterm-256color linux \
       scoansi ansi rxvt dtterm nsterm screen"`; \
       do { { infocmp -1 -x $i | grep kcu | sed 's/^\s*//';}; \
       echo $i;} | paste -s ; done | sort -u
#endif
    esc_codes_ = {
        // -------------------------- kcu --------------------------
        // nsterm screen vt100 vt102 vt320 vt400
        // xterm xterm-16color xterm-256color xterm-new xterm-r5 xterm-r6
        { "\033OA", Key::Up },
        { "\033OB", Key::Down },
        { "\033OC", Key::Right },
        { "\033OD", Key::Left },
        // ansi dtterm linux rxvt scoansi vt220
        { "\033[A", Key::Up },
        { "\033[B", Key::Down },
        { "\033[C", Key::Right },
        { "\033[D", Key::Left },

        // -------------------------- kUP5\|kDN5\|kLFT5\|kRIT5 --------------------------
        // xterm xterm-16color xterm-256color xterm-new
        { "\033[1;5A", Key::Ctrl_Up },
        { "\033[1;5B", Key::Ctrl_Down },
        { "\033[1;5C", Key::Ctrl_Right },
        { "\033[1;5D", Key::Ctrl_Left },
        // rxvt
        { "\033Oa", Key::Ctrl_Up },
        { "\033Ob", Key::Ctrl_Down },
        { "\033Oc", Key::Ctrl_Right },
        { "\033Od", Key::Ctrl_Left },

        // -------------------------- kUP\|kDN\|kLFT\|kRIT --------------------------
        // xterm xterm-16color xterm-256color xterm-new
        { "\033[1;2A", Key::Shift_Up },
        { "\033[1;2B", Key::Shift_Down },
        { "\033[1;2C", Key::Shift_Right },
        { "\033[1;2D", Key::Shift_Left },
        // rxvt
        { "\033[a", Key::Shift_Up },
        { "\033[b", Key::Shift_Down },
        { "\033[c", Key::Shift_Right },
        { "\033[d", Key::Shift_Left },

        // -------------------------- khome --------------------------
        // nsterm xterm xterm-16color xterm-256color xterm-new
        { "\033OH", Key::Home },
        // ansi scoansi
        { "\033[H", Key::Home },
        // linux screen vt320 xterm-r5
        { "\033[1~", Key::Home },
        // rxvt
        { "\033[7~", Key::Home },

        // -------------------------- kend --------------------------
        // nsterm xterm xterm-16color xterm-256color xterm-new
        { "\033OF", Key::End },
        // scoansi
        { "\033[F", Key::End },
        // linux screen xterm-r5
        { "\033[4~", Key::End },
        // rxvt
        { "\033[8~", Key::End },

        // -------------------------- kich1 --------------------------
        // dtterm linux rxvt screen vt220 vt320 xterm xterm-16color
        // xterm-256color xterm-new xterm-r5 xterm-r6
        { "\0332~", Key::Insert },
        // ansi scoansi
        { "\033[L", Key::Insert },

        // -------------------------- kdch1 --------------------------
        // dtterm linux nsterm rxvt screen vt220 vt320 xterm
        // xterm-16color xterm-256color xterm-new xterm-r5 xterm-r6
        { "\033[3~", Key::Delete },

        // -------------------------- kpp\|knp --------------------------
        // dtterm linux nsterm rxvt screen vt220 vt320 xterm
        // xterm-16color xterm-256color xterm-new xterm-r5 xterm-r6
        { "\033[5~", Key::Page_Up },
        { "\033[6~", Key::Page_Down },
        // scoansi
        { "\033[I", Key::Page_Up },
        { "\033[G", Key::Page_Down },

        // -------------------------- kf1-kf12 --------------------------
        // nsterm screen vt100 vt102 vt220 vt320 vt400 xterm
        // xterm-16color xterm-256color xterm-new
        { "\033OP", Key::F1 },
        { "\033OQ", Key::F2 },
        { "\033OR", Key::F3 },
        { "\033OS", Key::F4 },
        // vt100 vt102
        { "\033Ot", Key::F5 },
        { "\033Ou", Key::F6 },
        { "\033Ov", Key::F7 },
        { "\033Ol", Key::F8 },
        { "\033Ow", Key::F9 },
        { "\033Ox", Key::F10 },
        // dtterm rxvt xterm-r5 xterm-r6
        { "\033[11~", Key::F1 },
        { "\033[12~", Key::F2 },
        { "\033[13~", Key::F3 },
        { "\033[14~", Key::F4 },
        { "\033[15~", Key::F5 },
        { "\033[17~", Key::F6 },
        { "\033[18~", Key::F7 },
        { "\033[19~", Key::F8 },
        { "\033[20~", Key::F9 },
        { "\033[21~", Key::F10 },
        { "\033[23~", Key::F11 },
        { "\033[24~", Key::F12 },
        // linux
        { "\033[[A", Key::F1 },
        { "\033[[B", Key::F2 },
        { "\033[[C", Key::F3 },
        { "\033[[D", Key::F4 },
        { "\033[[E", Key::F5 },
        // scoansi
        { "\033[M", Key::F1 },
        { "\033[N", Key::F2 },
        { "\033[O", Key::F3 },
        { "\033[P", Key::F4 },
        { "\033[Q", Key::F5 },
        { "\033[R", Key::F6 },
        { "\033[S", Key::F7 },
        { "\033[T", Key::F8 },
        { "\033[U", Key::F9 },
        { "\033[V", Key::F10 },
        { "\033[W", Key::F11 },
        { "\033[X", Key::F12 },

        // -------------------------- kf13-kf24 --------------------------
        // xterm xterm-16color xterm-256color xterm-new
        { "\033[1;2P", Key::Shift_F1 },
        { "\033[1;2Q", Key::Shift_F2 },
        { "\033[1;2R", Key::Shift_F3 },
        { "\033[1;2S", Key::Shift_F4 },
        { "\033[15;2~", Key::Shift_F5 },
        { "\033[17;2~", Key::Shift_F6 },
        { "\033[18;2~", Key::Shift_F7 },
        { "\033[19;2~", Key::Shift_F8 },
        { "\033[20;2~", Key::Shift_F9 },
        { "\033[21;2~", Key::Shift_F10 },
        { "\033[23;2~", Key::Shift_F11 },
        { "\033[24;2~", Key::Shift_F12 },

        // dtterm linux nsterm rxvt vt320 xterm-r6 vt220
        { "\033[25~", Key::Shift_F1 },
        { "\033[26~", Key::Shift_F2 },
        { "\033[28~", Key::Shift_F3 },
        { "\033[29~", Key::Shift_F4 },
        { "\033[31~", Key::Shift_F5 },
        { "\033[32~", Key::Shift_F6 },
        { "\033[33~", Key::Shift_F7 },
        { "\033[34~", Key::Shift_F8 },
        { "\033[23$", Key::Shift_F9 },
        { "\033[24$", Key::Shift_F10 },
        //{ "\033[11\^", Key::Shift_F11 },
        //{ "\033[12\^", Key::Shift_F12 },

        // scoansi
        { "\033[Y", Key::Shift_F1 },
        // { "\033[a", Key::Shift_F3 },
        // { "\033[b", Key::Shift_F4 },
        // { "\033[c", Key::Shift_F5 },
        // { "\033[d", Key::Shift_F6 },
        { "\033[e", Key::Shift_F7 },
        { "\033[f", Key::Shift_F8 },
        { "\033[g", Key::Shift_F9 },
        { "\033[h", Key::Shift_F10 },
        { "\033[i", Key::Shift_F11 },
        { "\033[j", Key::Shift_F12 },

        // -------------------------- kf25-kf36 --------------------------
        // xterm xterm-16color xterm-256color xterm-new
        { "\033[1;5P", Key::Ctrl_F1 },
        { "\033[1;5Q", Key::Ctrl_F2 },
        { "\033[1;5R", Key::Ctrl_F3 },
        { "\033[1;5S", Key::Ctrl_F4 },
        { "\033[15;5~", Key::Ctrl_F5 },
        { "\033[17;5~", Key::Ctrl_F6 },
        { "\033[18;5~", Key::Ctrl_F7 },
        { "\033[19;5~", Key::Ctrl_F8 },
        { "\033[20;5~", Key::Ctrl_F9 },
        { "\033[21;5~", Key::Ctrl_F10 },
        { "\033[23;5~", Key::Ctrl_F11 },
        { "\033[24;5~", Key::Ctrl_F12 },

        // rxvt
        //{ "\033[13\^", Key::Ctrl_F1 },
        //{ "\033[14\^", Key::Ctrl_F2 },
        //{ "\033[15\^", Key::Ctrl_F3 },
        //{ "\033[17\^", Key::Ctrl_F4 },
        //{ "\033[18\^", Key::Ctrl_F5 },
        //{ "\033[19\^", Key::Ctrl_F6 },
        //{ "\033[20\^", Key::Ctrl_F7 },
        //{ "\033[21\^", Key::Ctrl_F8 },
        //{ "\033[23\^", Key::Ctrl_F9 },
        //{ "\033[24\^", Key::Ctrl_F10 },
        //{ "\033[25\^", Key::Ctrl_F11 },
        //{ "\033[26\^", Key::Ctrl_F12 },

        // scoansi
        { "\033[k", Key::Ctrl_F1 },
        { "\033[l", Key::Ctrl_F2 },
        { "\033[m", Key::Ctrl_F3 },
        { "\033[n", Key::Ctrl_F4 },
        { "\033[o", Key::Ctrl_F5 },
        { "\033[p", Key::Ctrl_F6 },
        { "\033[q", Key::Ctrl_F7 },
        { "\033[r", Key::Ctrl_F8 },
        { "\033[s", Key::Ctrl_F9 },
        { "\033[t", Key::Ctrl_F10 },
        { "\033[u", Key::Ctrl_F11 },
        { "\033[v", Key::Ctrl_F12 },
        // -------------------------- kcbt --------------------------
        // ansi scoansi screen xterm xterm-16color xterm-256color xterm-new linux nsterm rxvt
        { "\033[Z", Key::Shift_Tab },

        // Alt + character
        { "\033a", Key::Alt_a },
        { "\033b", Key::Alt_b },
        { "\033c", Key::Alt_c },
        { "\033d", Key::Alt_d },
        { "\033e", Key::Alt_e },
        { "\033f", Key::Alt_f },
        { "\033g", Key::Alt_g },
        { "\033h", Key::Alt_h },
        { "\033i", Key::Alt_i },
        { "\033j", Key::Alt_j },
        { "\033k", Key::Alt_k },
        { "\033l", Key::Alt_l },
        { "\033m", Key::Alt_m },
        { "\033n", Key::Alt_n },
        { "\033o", Key::Alt_o },
        { "\033p", Key::Alt_p },
        { "\033q", Key::Alt_q },
        { "\033r", Key::Alt_r },
        { "\033s", Key::Alt_s },
        { "\033t", Key::Alt_t },
        { "\033u", Key::Alt_u },
        { "\033v", Key::Alt_v },
        { "\033w", Key::Alt_w },
        { "\033x", Key::Alt_x },
        { "\033y", Key::Alt_y },
        { "\033z", Key::Alt_z },

        { "\033A", Key::Alt_A },
        { "\033B", Key::Alt_B },
        { "\033C", Key::Alt_C },
        { "\033D", Key::Alt_D },
        { "\033E", Key::Alt_E },
        { "\033F", Key::Alt_F },
        { "\033G", Key::Alt_G },
        { "\033H", Key::Alt_H },
        { "\033I", Key::Alt_I },
        { "\033J", Key::Alt_J },
        { "\033K", Key::Alt_K },
        { "\033L", Key::Alt_L },
        { "\033M", Key::Alt_M },
        { "\033N", Key::Alt_N },
        { "\033O", Key::Alt_O },
        { "\033P", Key::Alt_P },
        { "\033Q", Key::Alt_Q },
        { "\033R", Key::Alt_R },
        { "\033S", Key::Alt_S },
        { "\033T", Key::Alt_T },
        { "\033U", Key::Alt_U },
        { "\033V", Key::Alt_V },
        { "\033W", Key::Alt_W },
        { "\033X", Key::Alt_X },
        { "\033Y", Key::Alt_Y },
        { "\033Z", Key::Alt_Z },
    };
}

void Tty::restoreOrigTerminal() {
    if ( tcsetattr(term_stdin_, TCSANOW, &orig_term_) == -1 ) {
        Error::getInstance().appendError(utils::strFormat("%s:%d:%s", __FILE__, __LINE__, strerror(errno)));
        std::exit(EXIT_FAILURE);
    }
}

void Tty::setNewTerminal() {
    if ( tcsetattr(term_stdin_, TCSANOW, &new_term_) == -1 ) {
        Error::getInstance().appendError(utils::strFormat("%s:%d:%s", __FILE__, __LINE__, strerror(errno)));
        std::exit(EXIT_FAILURE);
    }
}

enum class KeyType
{
    Empty,
    Char,
    Control,
    EscCode
};

std::tuple<Key, std::string> Tty::getchar() {
    static char left_char = -1;
    static char last_char = -1;
    static KeyType last_type = KeyType::Empty;

    std::string esc_code;
    if ( left_char == '\033' ) {
        esc_code.push_back(left_char);
    }
    else if ( std::iscntrl(left_char) ) {
        auto key = left_char;
        left_char = -1;
        last_type = KeyType::Control;
        return std::make_tuple(static_cast<Key>(key), std::string());
    }

    bool has_esc = left_char == '\033';
    while ( running_.load(std::memory_order_relaxed) ) {
        char ch;
        auto n = read(term_stdin_, &ch, 1);
        if ( n > 0 ) {
            bool is_same = ch == last_char;
            bool valid = last_char != -1;
            last_char = ch;
            left_char = -1;
            if ( !has_esc ) {
                if ( std::iscntrl(ch) ) {
                    if ( last_type == KeyType::Char || ( valid && !is_same ) ) {
                        last_type = KeyType::Empty;
                        left_char = ch;
                        return std::make_tuple(Key::Timeout, std::string(1, ch));
                    }

                    if ( ch == '\033' ) {
                        has_esc = true;
                        esc_code.push_back(ch);
                    }
                    else {
                        last_type = KeyType::Control;
                        return std::make_tuple(static_cast<Key>(ch), std::string());
                    }
                }
                else {
                    last_type = KeyType::Char;
                    return std::make_tuple(Key::Char, std::string(1, ch));
                }
            }
            else {
                if ( std::iscntrl(ch) ) {
                    left_char = ch;
                    if ( esc_code.length() == 1 ) { // e.g. \033\033
                        last_type = KeyType::Control;
                        return std::make_tuple(Key::ESC, std::string());
                    }
                    else { // e.g. \033[A\033[B
                        last_type = KeyType::EscCode;
                        auto iter = esc_codes_.find(esc_code);
                        if ( iter != esc_codes_.end() ) {
                            return std::make_tuple(iter->second, std::move(esc_code));
                        }
                        else {
                            auto k = Key::Unknown;
                            auto xy = _mouseTracking(esc_code, k);
                            if ( k != Key::Unknown ) {
                                return std::make_tuple(k, std::move(xy));
                            }
                            else {
                                _getCursorPos(esc_code);
                                return std::make_tuple(Key::Unknown, std::string());
                            }
                        }
                    }
                }
                else {
                    esc_code.push_back(ch);
                    auto k = Key::Unknown;
                    auto xy = _mouseTracking(esc_code, k);
                    if ( k != Key::Unknown ) {
                        return std::make_tuple(k, std::move(xy));
                    }
                    if ( _getCursorPos(esc_code) ) {
                        return std::make_tuple(Key::Unknown, std::string());
                    }
                }
            }
        }
        else if ( n == 0 ) { // time out
            last_char = -1;
            left_char = -1;
            if ( !esc_code.empty() ) {
                if ( esc_code.length() == 1 ) {
                    last_type = KeyType::Control;
                    return std::make_tuple(Key::ESC, std::string());
                }
                else { // e.g. \033[A
                    last_type = KeyType::EscCode;
                    auto iter = esc_codes_.find(esc_code);
                    if ( iter != esc_codes_.end() ) {
                        return std::make_tuple(iter->second, std::move(esc_code));
                    }
                    else {
                        auto k = Key::Unknown;
                        auto xy = _mouseTracking(esc_code, k);
                        if ( k != Key::Unknown ) {
                            return std::make_tuple(k, std::move(xy));
                        }
                        else {
                            _getCursorPos(esc_code);
                            return std::make_tuple(Key::Unknown, std::string());
                        }
                    }
                }
            }
            else if ( last_type != KeyType::Empty ) {
                last_type = KeyType::Empty;
                return std::make_tuple(Key::Timeout, std::string());
            }
        }
        else { // n < 0
            Error::getInstance().appendError(utils::strFormat("%s:%d:%s", __FILE__, __LINE__, strerror(errno)));
            std::exit(EXIT_FAILURE);
        }
    }

    return std::make_tuple(Key::Exit, std::string());

}

// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Mouse-Tracking
std::string Tty::_mouseTracking(const std::string& esc_code, Key& key) const {
    using namespace std::chrono;

    static std::string orig_xy;
    static auto start_time = steady_clock::now();

    std::smatch sm;
    // mouse enabled by \033[?1000h
    if ( esc_code.length() == 6 && esc_code.compare(0, 3, "\033[M") == 0 ) {
        std::string xy;
        xy.append(std::to_string(esc_code[4] - 32));
        xy.append(";");
        xy.append(std::to_string(esc_code[5] - 32));
        switch ( esc_code[3] )
        {
        case 32:
            {
            auto cur_time = steady_clock::now();
            if ( orig_xy != xy || duration_cast<milliseconds>(cur_time - start_time).count() > 500 ) {
                orig_xy = xy;
                key = Key::Single_Click;
            }
            else {
                key = Key::Double_Click;
            }
            start_time = cur_time;
            }
            return xy;
        case 33:
            key = Key::Right_Click;
            return xy;
        case 34:
            key = Key::Middle_Click;
            return xy;
        case 35: // button release
            break;
        case 36:
            key = Key::Shift_LeftMouse;
            return xy;
        case 48:
            key = Key::Ctrl_LeftMouse;
            return xy;
        case 49: // ctrl + right click
            break;
        case 96:
            key = Key::Wheel_Up;
            return xy;
        case 97:
            key = Key::Wheel_Down;
            return xy;
        default:
            break;
        }
    }
    // mouse enabled by \033[?1006h
    else if ( std::regex_match(esc_code, sm, std::regex("\033\\[<(\\d+);(\\d+;\\d+)M")) ) {
        auto xy = sm.str(2);
        switch ( std::stol(sm.str(1)) )
        {
        case 0:
            {
            auto cur_time = steady_clock::now();
            if ( orig_xy != xy || duration_cast<milliseconds>(cur_time - start_time).count() > 500 ) {
                orig_xy = xy;
                key = Key::Single_Click;
            }
            else {
                key = Key::Double_Click;
            }
            start_time = cur_time;
            }
            return xy;
        case 1:
            key = Key::Right_Click;
            return xy;
        case 2:
            key = Key::Middle_Click;
            return xy;
        case 4:
            key = Key::Shift_LeftMouse;
            return xy;
        case 16:
            key = Key::Ctrl_LeftMouse;
            return xy;
        case 17: // ctrl + right click
            break;
        case 64:
            key = Key::Wheel_Up;
            return xy;
        case 65:
            key = Key::Wheel_Down;
            return xy;
        default:
            break;
        }
    }

    return esc_code;
}

bool Tty::_getCursorPos(const std::string& esc_code) {
    std::smatch sm;
    if ( cursor_pos_ == true && std::regex_match(esc_code, sm, std::regex("\033\\[(\\d+);(\\d+)R")) ) {
        cursor_line_ = stoi(sm.str(1));
        cursor_col_ = stoi(sm.str(2));
        std::lock_guard<std::mutex> lock(mutex_);
        cursor_pos_ = false;
        cursor_cond_.notify_one();
        return true;
    }
    return false;
}

} // end namespace leaf
