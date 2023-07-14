#include <unistd.h>
#include <sys/wait.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <fcntl.h>
#include <execinfo.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include <regex>

#include "app.h"
#include "utils.h"

namespace leaf
{

void SignalManager::installHandler() {
    struct sigaction act;

    act.sa_sigaction = SignalManager::catchSignal;
    sigemptyset (&act.sa_mask);
    act.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGFPE, &act, NULL);
    sigaction(SIGILL, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGABRT, &act, NULL);

    // To avoid zombie process
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, NULL);
}

void SignalManager::catchSignal(int sig, siginfo_t *info, void *ctxt) {
    auto& tty = Tty::getInstance();
    tty.enableAutoWrap_s();
    tty.disableMouse_s();
    tty.showCursor_s();
    if ( ConfigManager::getInstance().getConfigValue<ConfigType::Height>() == 0 ) {
        tty.disableAlternativeBuffer_s();
    }
    tty.restoreOrigTerminal_s();

    const char* msg = "";
    switch ( sig ) {
    case SIGBUS:
        msg = "catch signal SIGBUS...\n";
        break;
    case SIGFPE:
        msg = "catch signal SIGFPE...\n";
        break;
    case SIGILL:
        msg = "catch signal SIGILL...\n";
        break;
    case SIGSEGV:
        msg = "catch signal SIGSEGV...\n";
        break;
    case SIGABRT:
        msg = "catch signal SIGABRT...\n";
        break;
    default:
        break;
    }

    write(STDERR_FILENO, msg, strlen(msg));

    int mapfd = open("/proc/self/maps", O_RDONLY);
    if ( mapfd != -1 ) {
        const char* header = "\n******************** Memory map ********************\n\n";
        write(STDERR_FILENO, header, strlen(header));

        char buf[256];
        ssize_t n;

        while ( (n = read(mapfd, buf, sizeof(buf))) > 0 ) {
            write(STDERR_FILENO, buf, n);
        }

        close (mapfd);
    }

    void* arr[100];
    size_t size = backtrace(arr, 100);
    const char* header = "\n******************** Backtrace ********************\n\n";
    write(STDERR_FILENO, header, strlen(header));
    backtrace_symbols_fd(arr, size, STDERR_FILENO);

    auto pid = fork();
    if ( pid < 0 ) {
        msg = "fork error\n";
        write(STDERR_FILENO, msg, strlen(msg));
        std::_Exit(EXIT_FAILURE);
    }
    else if ( pid == 0 ) {
        header = "\n******************** addr2line Backtrace ********************\n\n";
        write(STDERR_FILENO, header, strlen(header));

        if ( dup2(STDERR_FILENO, STDOUT_FILENO) < 0 ) {
            perror("dup2()");
            std::_Exit(EXIT_FAILURE);
        }

        char** stacktrace = backtrace_symbols(arr, size);
        if ( stacktrace == nullptr ) {
            perror("backtrace_symbols()");
            std::_Exit(EXIT_FAILURE);
        }

        for ( size_t i = 0; i < size; ++i ) {
            std::string str(stacktrace[i]);
            std::smatch sm;
            std::regex_match(str, sm, std::regex("(.*)\\((.*)\\)\\s*\\[(.*)\\]"));
            auto exe = sm.str(1);
            auto addr1 = sm.str(2);
            auto addr2 = sm.str(3);
            if ( !addr1.empty() && addr1[0] == '+' ) {
                addr2 = addr1;
            }

            if ( system(("addr2line -apfCi -e " + exe + " " + addr2).c_str()) < 0 ) {
                perror("system()");
                std::_Exit(EXIT_FAILURE);
            }
        }

        free(stacktrace);

        header = "\n******************** gdb Backtrace ********************\n\n";
        write(STDERR_FILENO, header, strlen(header));

        char pid_buf[24];
        sprintf(pid_buf, "%d", getppid());
        execlp("gdb", "gdb", "--batch", "-n", "-iex",
               "set auto-load safe-path /", "-ex", "bt", "-p", pid_buf, nullptr);
    } else {
        if ( waitpid(pid, nullptr, 0) < 0 ) {
            msg = "waitpid error\n";
            write(STDERR_FILENO, msg, strlen(msg));
            std::_Exit(EXIT_FAILURE);
        }
    }

    // default behavior, so that a core file can be produced
    struct sigaction act;
    act.sa_handler = SIG_DFL;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(sig, &act, NULL);
    raise(sig);
}

Application::Application(int argc, char* argv[])
    : Configuration(argc, argv),
    result_content_(std::get<1>(previous_result_)),
    cpu_count_{ std::max(std::thread::hardware_concurrency(), 1u) },
    step_{ 100000 * cpu_count_ },
    fuzzy_engine_(cpu_count_),
    key_map_{
        { "Ctrl_@", Key::Ctrl_At },
        { "Ctrl_A", Key::Ctrl_A },
        { "Ctrl_B", Key::Ctrl_B },
        { "Ctrl_C", Key::Ctrl_C },
        { "Ctrl_D", Key::Ctrl_D },
        { "Ctrl_E", Key::Ctrl_E },
        { "Ctrl_F", Key::Ctrl_F },
        { "Ctrl_G", Key::Ctrl_G },
        { "Ctrl_H", Key::Ctrl_H },
        { "Ctrl_I", Key::Ctrl_I },
        { "Ctrl_J", Key::Ctrl_J },
        { "Ctrl_K", Key::Ctrl_K },
        { "Ctrl_L", Key::Ctrl_L },
        { "Ctrl_M", Key::Ctrl_M },
        { "Enter",  Key::Ctrl_M },
        { "Ctrl_N", Key::Ctrl_N },
        { "Ctrl_O", Key::Ctrl_O },
        { "Ctrl_P", Key::Ctrl_P },
        { "Ctrl_Q", Key::Ctrl_Q },
        { "Ctrl_R", Key::Ctrl_R },
        { "Ctrl_S", Key::Ctrl_S },
        { "Ctrl_T", Key::Ctrl_T },
        { "Ctrl_U", Key::Ctrl_U },
        { "Ctrl_V", Key::Ctrl_V },
        { "Ctrl_W", Key::Ctrl_W },
        { "Ctrl_X", Key::Ctrl_X },
        { "Ctrl_Y", Key::Ctrl_Y },
        { "Ctrl_Z", Key::Ctrl_Z },
        { "ESC",    Key::ESC },
        { "Ctrl_\\", Key::FS },
        { "Ctrl_]", Key::GS },
        { "Ctrl_^", Key::RS },
        { "Ctrl_/", Key::US },
        { "Backspace", Key::Backspace },

        { "Up", Key::Up },
        { "Down", Key::Down },
        { "Right", Key::Right },
        { "Left", Key::Left },
        { "Ctrl_Up", Key::Ctrl_Up },
        { "Ctrl_Down", Key::Ctrl_Down },
        { "Ctrl_Right", Key::Ctrl_Right },
        { "Ctrl_Left", Key::Ctrl_Left },
        { "Shift_Up", Key::Shift_Up },
        { "Shift_Down", Key::Shift_Down },
        { "Shift_Right", Key::Shift_Right },
        { "Shift_Left", Key::Shift_Left },
        { "Home", Key::Home },
        { "End", Key::End },
        { "Insert", Key::Insert },
        { "Delete", Key::Delete },
        { "Page_Up", Key::Page_Up },
        { "Page_Down", Key::Page_Down },
        { "F1", Key::F1 },
        { "F2", Key::F2 },
        { "F3", Key::F3 },
        { "F4", Key::F4 },
        { "F5", Key::F5 },
        { "F6", Key::F6 },
        { "F7", Key::F7 },
        { "F8", Key::F8 },
        { "F9", Key::F9 },
        { "F10", Key::F10 },
        { "F11", Key::F11 },
        { "F12", Key::F12 },
        { "Shift_F1", Key::Shift_F1 },
        { "Shift_F2", Key::Shift_F2 },
        { "Shift_F3", Key::Shift_F3 },
        { "Shift_F4", Key::Shift_F4 },
        { "Shift_F5", Key::Shift_F5 },
        { "Shift_F6", Key::Shift_F6 },
        { "Shift_F7", Key::Shift_F7 },
        { "Shift_F8", Key::Shift_F8 },
        { "Shift_F9", Key::Shift_F9 },
        { "Shift_F10", Key::Shift_F10 },
        { "Shift_F11", Key::Shift_F11 },
        { "Shift_F12", Key::Shift_F12 },
        { "Ctrl_F1", Key::Ctrl_F1 },
        { "Ctrl_F2", Key::Ctrl_F2 },
        { "Ctrl_F3", Key::Ctrl_F3 },
        { "Ctrl_F4", Key::Ctrl_F4 },
        { "Ctrl_F5", Key::Ctrl_F5 },
        { "Ctrl_F6", Key::Ctrl_F6 },
        { "Ctrl_F7", Key::Ctrl_F7 },
        { "Ctrl_F8", Key::Ctrl_F8 },
        { "Ctrl_F9", Key::Ctrl_F9 },
        { "Ctrl_F10", Key::Ctrl_F10 },
        { "Ctrl_F11", Key::Ctrl_F11 },
        { "Ctrl_F12", Key::Ctrl_F12 },
        { "Shift_Tab", Key::Shift_Tab },
        { "Single_Click", Key::Single_Click },
        { "Double_Click", Key::Double_Click },
        { "Right_Click", Key::Right_Click },
        { "Wheel_Down", Key::Wheel_Down },
        { "Wheel_Up", Key::Wheel_Up },
        { "Ctrl_LeftMouse", Key::Ctrl_LeftMouse },
        { "Shift_LeftMouse", Key::Shift_LeftMouse },

        { "Alt_a", Key::Alt_a }, { "Alt_A", Key::Alt_A },
        { "Alt_b", Key::Alt_b }, { "Alt_B", Key::Alt_B },
        { "Alt_c", Key::Alt_c }, { "Alt_C", Key::Alt_C },
        { "Alt_d", Key::Alt_d }, { "Alt_D", Key::Alt_D },
        { "Alt_e", Key::Alt_e }, { "Alt_E", Key::Alt_E },
        { "Alt_f", Key::Alt_f }, { "Alt_F", Key::Alt_F },
        { "Alt_g", Key::Alt_g }, { "Alt_G", Key::Alt_G },
        { "Alt_h", Key::Alt_h }, { "Alt_H", Key::Alt_H },
        { "Alt_i", Key::Alt_i }, { "Alt_I", Key::Alt_I },
        { "Alt_j", Key::Alt_j }, { "Alt_J", Key::Alt_J },
        { "Alt_k", Key::Alt_k }, { "Alt_K", Key::Alt_K },
        { "Alt_l", Key::Alt_l }, { "Alt_L", Key::Alt_L },
        { "Alt_m", Key::Alt_m }, { "Alt_M", Key::Alt_M },
        { "Alt_n", Key::Alt_n }, { "Alt_N", Key::Alt_N },
        { "Alt_o", Key::Alt_o }, { "Alt_O", Key::Alt_O },
        { "Alt_p", Key::Alt_p }, { "Alt_P", Key::Alt_P },
        { "Alt_q", Key::Alt_q }, { "Alt_Q", Key::Alt_Q },
        { "Alt_r", Key::Alt_r }, { "Alt_R", Key::Alt_R },
        { "Alt_s", Key::Alt_s }, { "Alt_S", Key::Alt_S },
        { "Alt_t", Key::Alt_t }, { "Alt_T", Key::Alt_T },
        { "Alt_u", Key::Alt_u }, { "Alt_U", Key::Alt_U },
        { "Alt_v", Key::Alt_v }, { "Alt_V", Key::Alt_V },
        { "Alt_w", Key::Alt_w }, { "Alt_W", Key::Alt_W },
        { "Alt_x", Key::Alt_x }, { "Alt_X", Key::Alt_X },
        { "Alt_y", Key::Alt_y }, { "Alt_Y", Key::Alt_Y },
        { "Alt_z", Key::Alt_z }, { "Alt_Z", Key::Alt_Z }
    }, op_map_{
        { "Exit",                   Operation::Exit },
        { "Accept",                 Operation::Accept },
        { "Backspace",              Operation::Backspace },
        { "Delete",                 Operation::Delete },
        { "ClearLeftCharacters",    Operation::ClearLeft },
        { "DeleteLeftWord",         Operation::DeleteLeftWord },
        { "Page_Up",                Operation::PageUp },
        { "Page_Down",              Operation::PageDown },
        { "CursorMoveLeft",         Operation::CursorMoveLeft },
        { "CursorMoveRight",        Operation::CursorMoveRight },
        { "CursorMoveToBegin",      Operation::CursorMoveToBegin },
        { "CursorMoveToEnd",        Operation::CursorMoveToEnd },
    }, key_op_map_{
        { Key::ESC,             Operation::Exit },
        { Key::Ctrl_M,          Operation::Accept },
        { Key::Double_Click,    Operation::Accept },
        { Key::Backspace,       Operation::Backspace },
        { Key::Delete,          Operation::Delete },
        { Key::Ctrl_U,          Operation::ClearLeft },
        { Key::Ctrl_W,          Operation::DeleteLeftWord },
        { Key::Page_Up,         Operation::PageUp },
        { Key::Page_Down,       Operation::PageDown },
        { Key::Left,            Operation::CursorMoveLeft },
        { Key::Right,           Operation::CursorMoveRight },
        { Key::Home,            Operation::CursorMoveToBegin },
        { Key::End,             Operation::CursorMoveToEnd },
        { Key::Ctrl_J,          Operation::CursorLineMoveDown },
        { Key::Ctrl_K,          Operation::CursorLineMoveUp },
        { Key::Single_Click,    Operation::Single_Click },
        { Key::Double_Click,    Operation::Double_Click },
        //{ Key::Right_Click,     Operation::Right_Click },
        //{ Key::Middle_Click,    Operation::Middle_Click },
        { Key::Wheel_Down,      Operation::Wheel_Down },
        { Key::Wheel_Up,        Operation::Wheel_Up },
        //{ Key::Ctrl_LeftMouse,  Operation::Ctrl_LeftMouse },
        //{ Key::Shift_LeftMouse, Operation::Shift_LeftMouse },
    }, task_map_{
        { Operation::CursorLineMoveUp,
            [this] { ui_queue_.put([this] { tui_.scrollUp<MainWindow>(); }); }
        },
        { Operation::CursorLineMoveDown,
            [this] { ui_queue_.put([this] { tui_.scrollDown<MainWindow>(); }); }
        },
        { Operation::PageUp,
            [this] { ui_queue_.put([this] { tui_.pageUp<MainWindow>(); }); }
        },
        { Operation::PageDown,
            [this] { ui_queue_.put([this] { tui_.pageDown<MainWindow>(); }); }
        },
        { Operation::Single_Click,
            [this] { ui_queue_.put([this, yx=current_yx_] { tui_.singleClick(yx); }); }
        },
        { Operation::Wheel_Down,
            [this] { ui_queue_.put([this, yx=current_yx_] { tui_.wheelDown(yx); }); }
        },
        { Operation::Wheel_Up,
            [this] { ui_queue_.put([this, yx=current_yx_] { tui_.wheelUp(yx); }); }
        },
    }, cmdline_task_map_{
        { Operation::Backspace,
            [this](std::string& pattern, uint32_t& cursor_pos) {
                if ( cursor_pos > 0 ) {
                    already_zero_ = false;
                    auto orig_cursor_pos = cursor_pos;
                    do {
                        --cursor_pos;
                    } while ( cursor_pos > 0 && !utils::is_utf8_boundary(pattern[cursor_pos]) );
                    pattern.erase(cursor_pos, orig_cursor_pos - cursor_pos);

                    cmdline_queue_.put([this, pattern, cursor_pos] {
                        tui_.updateCmdline(pattern, cursor_pos);
                    });
                }
            }
        },
        { Operation::Delete,
            [this](std::string& pattern, uint32_t& cursor_pos) {
                if ( pattern.length() ) {
                    already_zero_ = false;
                    if ( cursor_pos < pattern.length() ) {
                        auto orig_cursor_pos = cursor_pos;
                        do {
                            ++cursor_pos;
                        } while ( cursor_pos < pattern.length()
                                  && !utils::is_utf8_boundary(pattern[cursor_pos]) );
                        pattern.erase(orig_cursor_pos, cursor_pos - orig_cursor_pos);
                        cursor_pos = orig_cursor_pos;
                    }
                    else {
                        auto orig_cursor_pos = cursor_pos;
                        do {
                            --cursor_pos;
                        } while ( cursor_pos > 0 && !utils::is_utf8_boundary(pattern[cursor_pos]) );
                        pattern.erase(cursor_pos, orig_cursor_pos - cursor_pos);
                    }
                    cmdline_queue_.put([this, pattern, cursor_pos] {
                        tui_.updateCmdline(pattern, cursor_pos);
                    });
                }
            }
        },
        { Operation::ClearLeft,
            [this](std::string& pattern, uint32_t& cursor_pos) {
                if ( cursor_pos > 0 ) {
                    already_zero_ = false;
                    pattern.erase(0, cursor_pos);
                    cursor_pos = 0;
                    cmdline_queue_.put([this, pattern, cursor_pos] {
                        tui_.updateCmdline(pattern, cursor_pos);
                    });
                    _shorten(pattern, cursor_pos);
                }
                else {
                    already_zero_ = true;
                }
            }
        },
        { Operation::DeleteLeftWord,
            [this](std::string& pattern, uint32_t& cursor_pos) {
                auto orig_cursor_pos = cursor_pos;
                while ( cursor_pos > 0 && pattern[cursor_pos-1] == ' ' ) {
                    --cursor_pos;
                }
                while ( cursor_pos > 0 && pattern[cursor_pos-1] != ' ' ) {
                    --cursor_pos;
                }
                pattern.erase(cursor_pos, orig_cursor_pos - cursor_pos);
                cmdline_queue_.put([this, pattern, cursor_pos] {
                    tui_.updateCmdline(pattern, cursor_pos);
                });

                if ( cursor_pos > 0 ) {
                    already_zero_ = false;
                }
            }
        },
        { Operation::CursorMoveLeft,
            [this](std::string& pattern, uint32_t& cursor_pos) {
                if ( cursor_pos > 0 ) {
                    already_zero_ = false;
                    do {
                        --cursor_pos;
                    } while ( cursor_pos > 0 && !utils::is_utf8_boundary(pattern[cursor_pos]) );

                    cmdline_queue_.put([this, pattern, cursor_pos] {
                        tui_.updateCmdline(pattern, cursor_pos);
                    });
                }
                else {
                    already_zero_ = true;
                }
            }
        },
        { Operation::CursorMoveRight,
            [this](std::string& pattern, uint32_t& cursor_pos) {
                if ( cursor_pos < pattern.length() ) {
                    do {
                        ++cursor_pos;
                    } while ( cursor_pos < pattern.length()
                              && !utils::is_utf8_boundary(pattern[cursor_pos]) );

                    cmdline_queue_.put([this, pattern, cursor_pos] {
                        tui_.updateCmdline(pattern, cursor_pos);
                    });
                }
            }
        },
        { Operation::CursorMoveToBegin,
            [this](std::string& pattern, uint32_t& cursor_pos) {
                if ( cursor_pos > 0 ) {
                    cursor_pos = 0;
                    already_zero_ = true;
                    cmdline_queue_.put([this, pattern, cursor_pos] {
                        tui_.updateCmdline(pattern, cursor_pos);
                    });
                }
            }
        },
        { Operation::CursorMoveToEnd,
            [this](std::string& pattern, uint32_t& cursor_pos) {
                cursor_pos = pattern.length();
                cmdline_queue_.put([this, pattern, cursor_pos] {
                    tui_.updateCmdline(pattern, cursor_pos);
                });
            }
        },
    }
{
    content_.reserve(255);
}

#ifdef __APPLE__
constexpr int SIG_UserDefined = SIGUSR1;
#else
const int SIG_UserDefined = SIGRTMIN+8;
#endif

void Application::_handleSignal() {
    sigset_t waitset;
    sigemptyset(&waitset);

    sigaddset(&waitset, SIGINT);    // ctrl-c
    sigaddset(&waitset, SIGQUIT);
    sigaddset(&waitset, SIGTERM);
    sigaddset(&waitset, SIGTSTP);
    sigaddset(&waitset, SIGCONT);
    sigaddset(&waitset, SIGWINCH);
    sigaddset(&waitset, SIG_UserDefined);

    while ( running_.load(std::memory_order_relaxed) )  {
        int sig;
        int ret = sigwait(&waitset, &sig);
        if ( ret != -1 ) {
            switch ( sig )
            {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                Tty::getInstance().exit();
                return;
            case SIGTSTP:
                Cleanup::getInstance().doWork(false);
                kill(getpid(), SIGSTOP);
                sigstop_ = true;
                break;
            case SIGCONT:
                if ( sigstop_ ) {
                    sigstop_ = false;
                    Tty::getInstance().setNewTerminal();
                    _resume();
                }
                break;
            case SIGWINCH:
                break;
            default:
                if ( sig == SIG_UserDefined ) {
                    return;
                }
                break;
            }
        }
        //// for sigwaitinfo
        //else if ( errno == EINTR ) {
        //    continue;
        //}
        else {
            Error::getInstance().appendError(ErrorMessage);
            std::exit(EXIT_FAILURE);
        }
    }
}

void Application::_readConfig() {

}

void Application::start() {
    std::thread sig_handler(&Application::_handleSignal, this);
    std::thread reader(&Application::_readData, this);
    std::thread input(&Application::_input, this);
    std::thread ui(&Application::_doWork, this, std::ref(ui_queue_));
    std::thread cmdline(&Application::_doWork, this, std::ref(cmdline_queue_));
    std::thread show_flag(&Application::_showFlag, this);

    _doWork(task_queue_);

    show_flag.join();
    sig_handler.join();
    reader.join();
    input.join();
    ui.join();
    cmdline.join();
}

int Application::_exec(const char* cmd) {
    int fd[2];

    if ( pipe(fd) < 0 ) {
        Error::getInstance().appendError(ErrorMessage);
        std::exit(EXIT_FAILURE);
        return -1;
    }

    auto pid = fork();
    if ( pid < 0 ) {
        close(fd[0]);
        close(fd[1]);
        Error::getInstance().appendError(ErrorMessage);
        std::exit(EXIT_FAILURE);
        return -1;
    }
    else if ( pid == 0 ) {
#ifdef __linux__
        prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
        close(fd[0]);
        if ( fd[1] != STDOUT_FILENO ) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
        }
        execl("/bin/sh", "sh", "-c", cmd, nullptr);
        std::exit(127);
    }

    close(fd[1]);

    return fd[0];
}

void Application::_readData() {
    using namespace std::chrono;

    struct FdRAII
    {
        explicit FdRAII(int fd) : fd(fd) {}
        ~FdRAII() {
            if ( fd != STDIN_FILENO ) {
                close(fd);
            }
        }
        int fd;
    };

    FdRAII read_fd = FdRAII(STDIN_FILENO);
    if ( isatty(STDIN_FILENO) ) {
#ifdef __APPLE__
        auto cmd = "find . -name \".\" -o -name \".*\" -prune -o -type f -print 2>/dev/null | cut -b3-";
#else
        auto cmd = "find . -name \".\" -o -name \".*\" -prune -o -type f -printf \"%P\n\" 2>/dev/null";
#endif
        read_fd.fd = _exec(cmd);
    }

    BufferStorage storage;
    auto start_time = steady_clock::now();
    char buffer[BufferLen];
    while ( running_ ) {
        auto len = read(read_fd.fd, buffer, sizeof(buffer));
        if ( len < 0 ) {
            Error::getInstance().appendError(ErrorMessage);
            std::exit(EXIT_FAILURE);
        }
        else if ( len == 0 ) {
            // indicate the end
            storage.put(std::make_shared<DataBuffer>());
            task_queue_.put([this, s=std::move(storage)]() mutable {
                _processData(std::move(s));
            });

            break;
        }
        storage.put(std::make_shared<DataBuffer>(buffer, len));
        auto end_time = steady_clock::now();
        if ( duration_cast<milliseconds>(end_time - start_time).count() > 50 ) {
            start_time = end_time;
            task_queue_.put([this, s=std::move(storage)]() mutable {
                _processData(std::move(s));
            });
        }
    }
}

void Application::_processData(BufferStorage&& storage) {
    static char eol = '\0';
    for ( auto& sp_buffer : storage.getBuffers() ) {
        // end
        if ( sp_buffer->len == 0 ) {
            if ( !incomplete_str_.empty() ) {
                DataBufferPtr data = std::make_shared<DataBuffer>(incomplete_str_.c_str(),
                                                                  incomplete_str_.length());
                buffer_storage_.put(data);
                incomplete_str_.clear();
                content_.push_back(makeConstString(data->buffer, data->len));
            }

            flag_running_ = false;
            break;
        }
        uint32_t i = 0;
        if ( !incomplete_str_.empty() ) {
            for ( ; i < sp_buffer->len; ++i ) {
                if ( sp_buffer->buffer[i] == '\r' || sp_buffer->buffer[i] == '\n' ) {
                    auto incomplete_len = incomplete_str_.length();
                    uint32_t str_len = incomplete_len + i;
                    DataBufferPtr data = std::make_shared<DataBuffer>(str_len);
                    memcpy(data->buffer, incomplete_str_.c_str(), incomplete_len);
                    if ( i > 0 ) {
                        memcpy(data->buffer + incomplete_len, sp_buffer->buffer, i);
                    }
                    buffer_storage_.put(data);
                    incomplete_str_.clear();
                    content_.push_back(makeConstString(data->buffer, str_len));
                    eol = sp_buffer->buffer[i];
                    ++i;    // point to next character
                    break;
                }
            }

            if ( i == sp_buffer->len ) {
                // no '\r' or '\n' found
                if ( eol == '\0' ) {
                    incomplete_str_.append(sp_buffer->buffer, sp_buffer->len);
                }
                continue;
            }
        }

        if ( eol == '\r' && i < sp_buffer->len && sp_buffer->buffer[i] == '\n' ) {
            ++i;
        }

        eol = '\0';

        auto start = sp_buffer->buffer + i;
        for ( ; i < sp_buffer->len; ++i ) {
            if ( sp_buffer->buffer[i] == '\r' || sp_buffer->buffer[i] == '\n' ) {
                content_.push_back(makeConstString(start, sp_buffer->buffer + i - start));
                if ( sp_buffer->buffer[i] == '\r' ) {
                    if ( i + 1 == sp_buffer->len ) {
                        eol = '\r';
                    }
                    else if ( sp_buffer->buffer[i + 1] == '\n' ) {
                        ++i;
                    }
                }
                start = sp_buffer->buffer + i + 1;
            }
        }

        if ( start < sp_buffer->buffer + sp_buffer->len ) {
            incomplete_str_ = std::string(start, sp_buffer->buffer + sp_buffer->len - start);
        }
    }

    buffer_storage_.extend(std::move(storage));

    if ( !pattern_.empty() ) {
        _search(true);
    }
    else {
        result_content_size_.store(content_.size(), std::memory_order_relaxed);
        cmdline_queue_.put([this, result_size=content_.size(), total_size=content_.size()] {
            tui_.updateLineInfo(result_size, total_size);
        });
        static bool enough_size = false;
        if ( enough_size == false ) {
            if ( content_.size() >= tui_.getCoreHeight<MainWindow>() ) {
                enough_size = true;
            }
            ui_queue_.put([this]{ _initBuffer(); });
        }
    }
}

void Application::_input() {
    using namespace std::chrono;
    using Ms = std::chrono::milliseconds;
    using TimePoint = decltype(steady_clock::now());

    TimePoint cur_time;
    TimePoint start_time;
    bool timeout = true;
    std::string pattern;
    uint32_t cursor_pos = 0;
    Operation last_op = Operation::Invalid;
    while ( true ) {
        auto tup = Tty::getInstance().getchar();
        if ( timeout == true ) {
            timeout = false;
            start_time = steady_clock::now();
        }
        auto key = std::get<0>(tup);
        if ( key == Key::Ctrl_I ) { // tab
            normal_mode_ = !normal_mode_;
            cmdline_queue_.put([this] {
                tui_.redrawPrompt(normal_mode_);
            });
        }

        if ( normal_mode_ ) {
            if ( std::get<1>(tup) == "j" ) {
                key = Key::Ctrl_J;
            }
            else if ( std::get<1>(tup) == "k" ) {
                key = Key::Ctrl_K;
            }
            else if ( key == Key::Char ) {
                key = Key::Unknown;
            }
        }

        if ( key == Key::Char ) {
            last_op = Operation::Input;
            pattern.insert(cursor_pos, std::get<1>(tup));
            ++cursor_pos;
            cmdline_queue_.put([this, pattern, cursor_pos] {
                tui_.updateCmdline(pattern, cursor_pos);
            });

            cur_time = steady_clock::now();
            if ( result_content_size_.load(std::memory_order_relaxed) < 20000
                 || duration_cast<Ms>(cur_time - start_time).count() > 500 ) {
                start_time = cur_time;

                search_count_++;
                task_queue_.put([this, pattern] {
                    pattern_ = std::move(pattern);
                    _search(false);
                });
            }
        }
        else if ( key == Key::Timeout ) {
            timeout = true;
            switch ( last_op ) {
            case Operation::Input:
                if ( start_time != cur_time ) {
                    search_count_++;
                    task_queue_.put([this, pattern] {
                        pattern_ = std::move(pattern);
                        _search(false);
                    });
                }
                break;
            case Operation::Backspace:
            case Operation::Delete:
                if ( start_time != cur_time ) {
                    _shorten(pattern, cursor_pos);
                }
                break;
            default:
                break;
            }
        }
        else if ( key == Key::Exit ) {
            _notifyExit();
            break;
        }
        else {
            if ( key == Key::Single_Click || key == Key::Right_Click
                 || key == Key::Middle_Click || key == Key::Wheel_Down
                 || key == Key::Wheel_Up || key == Key::Ctrl_LeftMouse
                 || key == Key::Shift_LeftMouse ) {
                auto& esc_code = std::get<1>(tup);
                auto semicolon = esc_code.find(';');
                current_yx_.line = std::stol(esc_code.substr(semicolon+1));
                current_yx_.col = std::stol(esc_code.substr(0, semicolon));
            }

            auto iter = key_op_map_.find(key);
            if ( iter != key_op_map_.end() ) {
                auto op = iter->second;
                last_op = op;
                if ( op == Operation::Exit ) {
                    _notifyExit();
                    break;
                }
                else if ( op == Operation::Accept ) {
                    _notifyExit();
                    tui_.setAccept();
                    break;
                }

                auto it = cmdline_task_map_.find(op);
                if ( it != cmdline_task_map_.end() ) {
                    it->second(pattern, cursor_pos); // e.g. ctrl-u, backspace, left, right
                }

                if ( op == Operation::Backspace || op == Operation::Delete ) {
                    cur_time = steady_clock::now();
                    if ( duration_cast<Ms>(cur_time - start_time).count() > 200 ) {
                        start_time = cur_time;
                        _shorten(pattern, cursor_pos);
                    }
                }

                auto task_iter = task_map_.find(op);
                if ( task_iter != task_map_.end() ) {
                    task_queue_.put(task_iter->second);
                }
            }
            else {
                last_op = Operation::Invalid;
            }
        }
    }
}

void Application::_notifyExit() {
#ifdef __APPLE__
    kill(getpid(), SIG_UserDefined);
#else
    union sigval sigv;
    sigqueue(getpid(), SIG_UserDefined, sigv);
#endif

    running_ = false;

    ui_queue_.put(Task());
    task_queue_.put(Task());
    cmdline_queue_.put(Task());
}

void Application::_shorten(const std::string& pattern, uint32_t cursor_pos) {
    if ( already_zero_ ) {
        return;
    }

    if ( cursor_pos == 0 ) {
        already_zero_ = true;
    }
    else {
        already_zero_ = false;
    }

    if ( pattern.empty() ) {
        task_queue_.put([this] {
            result_content_size_.store(content_.size(), std::memory_order_relaxed);
            cmdline_queue_.put([this, content_size=content_.size()] {
                tui_.updateLineInfo(content_size, content_size);
            });
        });
        task_queue_.put([this] {
            index_ = 0;
            pattern_.clear();
            ui_queue_.put([this]{ _initBuffer(); });
        });
    }
    else {
        search_count_++;
        task_queue_.put([this, pattern] {
            pattern_ = std::move(pattern);
            index_ = 0;
            _search(false);
        });
    }

}

void Application::_search(bool is_continue) {
    if ( running_ == false ) { // stop continue
        return;
    }

    if ( !is_continue ) {
        search_count_--;
    }

    using namespace std::chrono;
    StrContainer::const_iterator source_begin;
    uint32_t content_size{ 0 };
    auto total_size{ content_.size() };
    StrContainer cur_content;
    std::function<void()> guard;

    if ( index_ == 0 ) {
        cb_content_.clear();
        result_content_.clear();
        index_ = std::min(step_, static_cast<decltype(step_)>(total_size));
        source_begin = content_.cbegin();
        content_size = index_;
    }
    else {
        uint32_t result_size = is_continue ? 0 : result_content_.size();
        if ( result_size >= step_ ) {
            source_begin = result_content_.cbegin();
            content_size = step_;
            if ( result_size > step_ ) {
                cb_content_.push_front(result_content_.cbegin() + step_, result_content_.cend());
            }
        }
        else {
            if ( result_size > 0 ) {
                cur_content.reserve(step_);
                cur_content.push_back(result_content_.cbegin(), result_content_.cend());
            }
            uint32_t cb_size = cb_content_.size();
            if ( cb_size >= step_ - result_size ) {
                if ( result_size == 0 ) {
                    source_begin = cb_content_.cbegin();
                    content_size = step_;
                    guard = [this, content_size] {
                        cb_content_.pop_front(content_size);
                    };
                }
                else {
                    auto last = cb_content_.cbegin() + (step_ - result_size);
                    cur_content.push_back(cb_content_.cbegin(), last);
                    cb_content_.pop_front(step_ - result_size);
                }
            }
            else {
                if ( cb_size > 0 ) {
                    cur_content.push_back(cb_content_.cbegin(), cb_content_.cend());
                    cb_content_.clear();
                }
                if ( index_ < total_size ) {
                    uint32_t offset = step_ - result_size - cb_size;
                    auto size = std::min(offset, static_cast<decltype(step_)>(total_size - index_));
                    if ( offset == step_ ) {
                        source_begin = content_.cbegin() + index_;
                        content_size = size;
                    }
                    else {
                        cur_content.push_back(content_.cbegin() + index_, content_.cbegin() + (index_ + size));
                    }
                    index_ += size;
                }
            }
        }

        if ( source_begin == nullptr ) {
            source_begin = cur_content.cbegin();
            content_size = cur_content.size();
        }
    }

    auto result = fuzzy_engine_.fuzzyMatch(source_begin, content_size, pattern_, preference_);
    if ( is_continue && result_content_.size() > 0 ) {
        result = fuzzy_engine_.merge(previous_result_, result);
    }

    {
        std::unique_lock<std::mutex> lock(result_mutex_);
        while ( access_count_ > 0 ) {
            result_cond_.wait(lock);
        }

        // sync with _updateResult()
        if ( !is_continue || (index_ >= total_size && cb_content_.size() == 0) ) {
            access_count_++;
        }
    }

    previous_result_ = std::move(result);
    result_content_size_.store(result_content_.size(), std::memory_order_relaxed);
    cmdline_queue_.put([this, result_size=result_content_.size(), total_size] {
        tui_.updateLineInfo(result_size, total_size);
    });

    // update result only when not continue or last continue
    if ( !is_continue || (index_ >= total_size && cb_content_.size() == 0) ) {
        ui_queue_.put([this, pattern=pattern_, result_size=result_content_.size()]{
            _updateResult(result_size, pattern);
        });
    }

    if ( guard ) {
        guard();
    }

    if ( (index_ < total_size || cb_content_.size() > 0) && search_count_ == 0 ) {
        _search(true);
    }

}

void Application::_doWork(BlockingQueue<Task>& q) {
    while ( running_ ) {
        auto task = q.take();
        if ( task ) {
            task();
        }
        else {
            break;
        }
    }
}

std::vector<HighlightString> Application::_generateHighlightStr(uint32_t first, uint32_t last,
                                                                const std::string& pattern) {
    std::vector<HighlightString> res;

    if ( first == last ) {
        return res;
    }
    res.reserve(last - first);

    auto highlights = fuzzy_engine_.getHighlights(result_content_.cbegin() + first,
                                                  last - first, pattern);
    const char* reset_color = "\033[0m";
    auto& match_color = tui_.getColor(HighlightGroup::Match0);
    auto& normal_color = tui_.getColor(HighlightGroup::Normal);

    auto max_width = tui_.getCoreWidth<MainWindow>() - indent_;
    if ( static_cast<int32_t>(max_width) <= 0 ) {
        return res;
    }
    auto iter = result_content_.cbegin() + first;
    auto end = result_content_.cbegin() + last;
    for (uint32_t i = 0; iter != end; ++iter, ++i ) {
        std::string s;
        s.reserve(iter->len << 1);
        auto& p_context = highlights[i];

        if ( iter->len <= max_width  ) {
            auto p = iter->str;
            for ( uint32_t j = 0; j < p_context->end_index; ++j ) {
                s.append(normal_color);
                s.append(p, iter->str + p_context->positions[j].col - p);
                p = iter->str + p_context->positions[j].col;
                s.append(match_color);
                s.append(p, p_context->positions[j].len);
                p += p_context->positions[j].len;
            }

            if ( auto len = iter->str + iter->len - p ) {
                s.append(normal_color);
                s.append(p, len);
            }
        }
        else {
            uint32_t right = p_context->end + std::min(iter->len - p_context->end, 10u);
            uint32_t left = 0;
            if ( right > max_width ) {
                left = right - max_width;
            }
            else {
                right = max_width;
            }

            auto p = iter->str + left;
            bool is_beginning = true;
            for ( uint32_t j = 0; j < p_context->end_index; ++j ) {
                auto col = p_context->positions[j].col;
                if ( col < left ) {
                    continue;
                }
                else if ( left == 0 || is_beginning == false ) {
                    is_beginning = false;
                    s.append(normal_color);
                    s.append(p, iter->str + p_context->positions[j].col - p);
                }
                else {
                    if ( col >= left + 2 ) {
                        is_beginning = false;

                        s.append(normal_color);
                        s.append("..");
                        s.append(p + 2, iter->str + p_context->positions[j].col - p - 2);
                    }
                    else {
                        continue;
                    }
                }

                p = iter->str + p_context->positions[j].col;
                s.append(match_color);
                s.append(p, p_context->positions[j].len);
                p += p_context->positions[j].len;
            }

            if ( auto len = right - p_context->end ) {
                s.append(normal_color);
                if ( right < iter->len ) {
                    s.append(p, len - 2);
                    s.append("..");
                }
                else {
                    s.append(p, len);
                }
            }
        }

        s.append(reset_color);

        res.emplace_back(std::move(s), *iter);
    }

    return res;
}

void Application::_initBuffer() {
    tui_.setBuffer<MainWindow>([this, indicator=0u]() mutable {
        auto height = tui_.getCoreHeight<MainWindow>();

        auto first = indicator;
        auto last = std::min(indicator + height,
                             static_cast<decltype(indicator)>(content_.size()));
        indicator = last;

        std::vector<HighlightString> res;
        res.reserve(last - first);

        const char* reset_color = "\033[0m";
        auto& normal_color = tui_.getColor(HighlightGroup::Normal);

        auto max_width = tui_.getCoreWidth<MainWindow>() - indent_;
        auto iter = content_.cbegin() + first;
        auto end = content_.cbegin() + last;
        if ( normal_color == reset_color ) {
            for ( ; iter != end; ++iter ) {
                if ( iter->len <= max_width  ) {
                    res.emplace_back(std::string(iter->str, iter->len), *iter);
                }
                else {
                    res.emplace_back(std::string(iter->str, max_width - 2) + "..", *iter);
                }
            }
        }
        else {
            for ( ; iter != end; ++iter ) {
                if ( iter->len <= max_width  ) {
                    res.emplace_back(normal_color + std::string(iter->str, iter->len) + reset_color,
                                     *iter);
                }
                else {
                    res.emplace_back(normal_color + std::string(iter->str, max_width - 2) + ".."
                                     + reset_color, *iter);
                }
            }
        }

        return res;
    });
}

void Application::_showFlag() {
    while ( flag_running_ && running_ ) {
        tui_.showFlag(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    tui_.showFlag(false);
}

// in ui_queue_ thread
void Application::_updateResult(uint32_t result_size, const std::string& pattern) {
    // [0, indicator) has been translated into highlight string
    tui_.setBuffer<MainWindow>([this, result_size, pattern, indicator=0u]() mutable {
        auto height = tui_.getCoreHeight<MainWindow>();

        auto first = indicator;
        auto last = std::min(indicator + height, result_size);
        indicator = last;

        return _generateHighlightStr(first, last, pattern);
    });

    {
        std::lock_guard<std::mutex> lock(result_mutex_);
        if ( access_count_ > 0 ) {
            access_count_--;
        }
        result_cond_.notify_one();
    }
}

void Application::_resume() {
    tui_.init(true);
    cmdline_queue_.put([this] {
        tui_.drawBorder();
        tui_.redrawPrompt(normal_mode_);
        tui_.updateCmdline(pattern_);
        tui_.updateLineInfo();
        tui_.setBuffer<MainWindow>();
    });
}

} // end namespace leaf
