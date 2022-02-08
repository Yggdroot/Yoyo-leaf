_Pragma("once");

#include <vector>
#include <memory>
#include <atomic>
#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <signal.h>

#include "tui.h"
#include "queue.h"
#include "error.h"
#include "constString.h"
#include "fuzzyEngine.h"
#include "configManager.h"

namespace leaf
{

constexpr uint32_t BufferLen = 64 * 1024;

enum class Operation
{
    Input,
    Exit,
    Accept,
    Backspace,
    Delete,
    ClearLeft,
    DeleteLeftWord,
    PageUp,
    PageDown,
    NormalMode,
    CursorMoveLeft,
    CursorMoveRight,
    CursorMoveToBegin,
    CursorMoveToEnd,
    CursorLineMoveUp,
    CursorLineMoveDown,
    Single_Click,
    Double_Click,
    Right_Click,
    Middle_Click,
    Wheel_Down,
    Wheel_Up,
    Ctrl_LeftMouse,
    Shift_LeftMouse,

    Invalid
};

struct DataBuffer
{
    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;
    DataBuffer(DataBuffer&&) = delete;
    DataBuffer& operator=(DataBuffer&&) = delete;

    DataBuffer() = default;

    explicit DataBuffer(uint32_t buf_len)
        : buffer(new char [buf_len]), len(buf_len) {}

    DataBuffer(const char* buf, uint32_t buf_len)
        : DataBuffer(buf_len) {
        memcpy(buffer, buf, buf_len);
    }

    ~DataBuffer() {
        delete [] buffer;
    }

    char*    buffer{ nullptr };
    uint32_t len{ 0 };
};

using DataBufferPtr = std::shared_ptr<DataBuffer>;

class BufferStorage
{
public:
    BufferStorage() {
        buffers_.reserve(8);
    }

    void put(DataBufferPtr sp_buffer) {
        buffers_.emplace_back(std::move(sp_buffer));
    }

    void extend(BufferStorage&& storage) {
        buffers_.insert(buffers_.end(), storage.buffers_.begin(), storage.buffers_.end());
    }

    bool empty() const noexcept {
        return buffers_.empty();
    }

    const std::vector<DataBufferPtr>& getBuffers() const noexcept {
        return buffers_;
    }

private:
    std::vector<DataBufferPtr> buffers_;
};

class SignalManager
{
public:
    SignalManager() {
        // make sure sigmask is set in main thread before any thread
        sigset_t sig_set;
        sigfillset(&sig_set);
        // If SIGBUS, SIGFPE, SIGILL, or SIGSEGV are generated while they are blocked,
        // the result is undefined, unless the signal was generated by kill(2), sigqueue(3), or raise(3).
        // On Linux, the default handler is invoked instead of the user handler.
        // so unblock them.
        sigdelset(&sig_set, SIGBUS);
        sigdelset(&sig_set, SIGFPE);
        sigdelset(&sig_set, SIGILL);
        sigdelset(&sig_set, SIGSEGV);
        if ( pthread_sigmask(SIG_BLOCK, &sig_set, nullptr) != 0 ) {
            Error::getInstance().appendError(utils::strFormat("%s:%d:%s", __FILE__, __LINE__, strerror(errno)));
            std::exit(EXIT_FAILURE);
        }

        installHandler();
    }

    void installHandler();

    static void catchSignal(int sig, siginfo_t *info, void *ctxt);
};

class Configuration : private SignalManager
{
public:
    Configuration(int argc, char* argv[]) {
        Error::getInstance(); // make sure Error object instance is created before Cleanup object instance
        ConfigManager::getInstance().loadConfig(argc, argv);
    }
};

class Application : public Configuration
{
public:
    Application(int argc, char* argv[]);
    void start();

private:
    using Task = std::function<void()>;

    void _handleSignal();
    void _readConfig();
    void _setNonBlocking(int fd);
    void _readData();
    void _processData(BufferStorage&& storage);
    void _input();
    void _shorten(const std::string& pattern, uint32_t cursor_pos);
    void _search(bool is_continue);
    void _doWork(BlockingQueue<Task>& q);
    void _updateResult(uint32_t result_size, const std::string& pattern);
    void _initBuffer();
    void _notifyExit();
    void _showFlag();
    void _resume();

    static int _exec(const char* cmd);

    std::vector<HighlightString> _generateHighlightStr(uint32_t first, uint32_t last,
                                                       const std::string& pattern);
private:
    Result        previous_result_;
    StrContainer& result_content_;
    StrContainer  content_;
    StrContainer  cb_content_;
    BufferStorage buffer_storage_;
    std::string   incomplete_str_;

    BlockingQueue<Task> task_queue_;
    BlockingQueue<Task> ui_queue_; // should be called in task_queue_ thread
    BlockingQueue<Task> cmdline_queue_;

    uint32_t      access_count_{ 0 };
    std::mutex    result_mutex_;
    std::condition_variable result_cond_;

    std::atomic<bool>     running_{ true };
    std::atomic<bool>     flag_running_{ true };
    std::atomic<uint32_t> search_count_{ 0 };
    std::atomic<uint32_t> result_content_size_{ 0 };

    Tui tui_;
    std::string pattern_;
    bool     already_zero_{ true };
    uint32_t index_{ 0 };
    uint32_t cpu_count_;
    const uint32_t step_;
    Point    current_yx_;
    uint32_t indent_{ ConfigManager::getInstance().getConfigValue<ConfigType::Indentation>() };
    bool     normal_mode_{ false };

    FuzzyEngine fuzzy_engine_;
    Preference  preference_{ ConfigManager::getInstance().getConfigValue<ConfigType::SortPreference>() };
    std::unordered_map<std::string, Key>       key_map_;
    std::unordered_map<std::string, Operation> op_map_;
    std::unordered_map<Key, Operation>         key_op_map_;
    std::unordered_map<Operation, Task>        task_map_;
    using CmdlineTask = std::function<void(std::string&, uint32_t&)>;
    std::unordered_map<Operation, CmdlineTask> cmdline_task_map_;

};

} // end namespace leaf
