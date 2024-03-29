_Pragma("once");

#include <memory>
#include <atomic>
#include <string>
#include <vector>
#include <functional>
#include "constString.h"
#include "color.h"
#include "tty.h"
#include "configManager.h"
#include "singleton.h"

namespace leaf
{

class Point
{
public:
    Point() = default;
    Point(uint32_t l, uint32_t c): line(l), col(c) {}
    uint32_t line;
    uint32_t col;
};

struct HighlightString
{
    template <typename T>
    HighlightString(T&& s, const ConstString& const_str): str(s), raw_str(const_str) {}

    std::string str;
    ConstString raw_str;
};

using Generator = std::function<std::vector<HighlightString>()>;

class Window
{
public:
    Window(const Point& tl, const Point& br, const Colorscheme& cs);
    virtual ~Window() = default;

    void _init(const Point& tl, const Point& br);

    uint32_t getHeight() const noexcept {
        return height_;
    }

    uint32_t getWidth() const noexcept {
        return width_;
    }

    uint32_t getCoreHeight() const noexcept {
        return core_height_;
    }

    uint32_t getCoreWidth() const noexcept {
        return core_width_;
    }

    void setBorder() {
        auto& border = ConfigManager::getInstance().getConfigValue<ConfigType::Border>();
        if ( border.empty() ) {
            return;
        }

        if ( border.find("T") != std::string::npos ) {
            border_mask_ |= 0b0001;
            core_height_ -= 1;
            core_top_left_.line += 1;
        }

        auto border_char_width = ConfigManager::getInstance().getBorderCharWidth();
        if ( border.find("R") != std::string::npos ) {
            border_mask_ |= 0b0010;
            core_width_ -= border_char_width;
        }

        if ( border.find("B") != std::string::npos ) {
            border_mask_ |= 0b0100;
            core_height_ -= 1;
        }

        if ( border.find("L") != std::string::npos ) {
            border_mask_ |= 0b1000;
            core_width_ -= border_char_width;
            core_top_left_.col += border_char_width;
        }
    }

    void setBuffer(Generator&& generator);
    void setBuffer();

    const std::vector<HighlightString>& getBuffer() const noexcept {
        return buffer_;
    }

    void scrollUp() {
        if ( is_reverse_ ) {
            _scrollDown();
        }
        else {
            _scrollUp();
        }
    }

    void _scrollUp();

    void scrollDown() {
        if ( is_reverse_ ) {
            _scrollUp();
        }
        else {
            _scrollDown();
        }
    }

    void _scrollDown();

    void pageUp() {
        if ( is_reverse_ ) {
            _pageDown();
        }
        else {
            _pageUp();
        }
    }

    void _pageUp();

    void pageDown() {
        if ( is_reverse_ ) {
            _pageUp();
        }
        else {
            _pageDown();
        }
    }

    void _pageDown();

    bool inWindow(const Point& point) const noexcept {
        return point.line >= top_left_.line
            && point.line <= bottom_right_.line
            && point.col >= top_left_.col
            && point.col <= bottom_right_.col;
    }

    bool inCoreWindow(const Point& point) const noexcept {
        if ( is_reverse_ ) {
            return point.line <= core_top_left_.line
                && point.line > core_top_left_.line - core_height_
                && point.col >= core_top_left_.col
                && point.col < core_top_left_.col + core_width_;
        }
        else {
            return point.line >= core_top_left_.line
                && point.line < core_top_left_.line + core_height_
                && point.col >= core_top_left_.col
                && point.col < core_top_left_.col + core_width_;
        }
    }

    void printAcceptedStrings() const noexcept {
        if ( !buffer_.empty() ) {
            printf("%s\n", std::string(buffer_[cursor_line_].raw_str.str,
                                       buffer_[cursor_line_].raw_str.len).c_str());
        }
    }

    void singleClick(const Point& yx) {
        if ( is_reverse_ ) {
            _updateCursorline(first_line_ + core_top_left_.line - yx.line);
        }
        else {
            _updateCursorline(first_line_ + yx.line - core_top_left_.line);
        }
    }

protected:
    void _render(uint32_t orig_cursorline_y);

    Point _getCursorlinePosition(uint32_t cursor_line) {
        if ( is_reverse_ ) {
            return Point(core_top_left_.line - (cursor_line - first_line_), core_top_left_.col + indent_);
        }
        else {
            return Point(core_top_left_.line + cursor_line - first_line_, core_top_left_.col + indent_);
        }
    }

    void _renderCursorline(uint32_t cursor_line);
    void _updateCursorline(uint32_t new_cursorline);

    virtual void _drawIndicator(uint32_t cursor_line_y) {}
    virtual void _clearIndicator(uint32_t cursor_line_y) {}
protected:
    Point    top_left_;
    Point    bottom_right_;
    Point    core_top_left_; // if is_reverse_, core_top_left_ is actually core bottom left
    uint32_t height_;
    uint32_t width_;
    uint32_t core_height_;
    uint32_t core_width_;
    std::vector<HighlightString> buffer_;
    Generator generator_;
    bool     is_reverse_{ ConfigManager::getInstance().getConfigValue<ConfigType::Reverse>() };
    uint32_t cursor_line_{ 0 }; // index of string under cursor line
    uint32_t first_line_{ 0 };  // index of string at the top of window
    uint32_t last_line_{ 0 };
    uint32_t border_mask_{ 0 };
    uint32_t indent_{ ConfigManager::getInstance().getConfigValue<ConfigType::Indentation>() };
    const Colorscheme& cs_;
};

class MainWindow : public Window
{
public:
    MainWindow(const Point& tl, const Point& br, const Colorscheme& cs): Window(tl, br, cs) {
        _setCmdline();
    }

    void reset(const Point& tl, const Point& br) {
        _init(tl, br);
        _setCmdline();
    }

    void display() const {
        drawBorder();
        _displayCmdline();
    }

    void updateLineInfo(uint32_t result_size, uint32_t total_size);
    void updateLineInfo() {
        updateLineInfo(result_size_, total_size_);
    }

    void updateCmdline(const std::string& pattern, uint32_t cursor_pos);
    void updateCmdline(const std::string& pattern) {
        updateCmdline(pattern, cursor_pos_);
    }

    void showFlag(bool show) {
        auto flag_col = flag_col_.load(std::memory_order_relaxed);
        if ( flag_col >= core_top_left_.col + core_width_ ) {
            return;
        }

        const std::string flag[] = { "◐", "◒", "◑", "◓" };
        if ( show ) {
            static uint32_t idx = 0;
            Tty::getInstance().addString(line_info_.line,
                                         flag_col,
                                         flag[(idx++) & 3],
                                         cs_.getColor(HighlightGroup::Flag));
        }
        else {
            Tty::getInstance().addString(line_info_.line,
                                         flag_col,
                                         std::string(2, ' '),
                                         cs_.getColor(HighlightGroup::Normal));
            Tty::getInstance().restoreCursorPosition();
            show_flag_.store(false, std::memory_order_relaxed);
        }
    }

    void redrawPrompt(bool normal_mode) {
        if ( normal_mode ) {
            Tty::getInstance().addString(cmdline_y_, core_top_left_.col, prompt_,
                                         cs_.getColor(HighlightGroup::NormalMode));
        }
        else {
            Tty::getInstance().addString(cmdline_y_, core_top_left_.col, prompt_,
                                         cs_.getColor(HighlightGroup::Prompt));
        }
        Tty::getInstance().restoreCursorPosition();
    }

    void drawBorder() const;

private:

    void _setCmdline();

    void _displayCmdline() const {
        Tty::getInstance().addString(cmdline_y_, core_top_left_.col, prompt_,
                                     cs_.getColor(HighlightGroup::Prompt));
        Tty::getInstance().saveCursorPosition();
    }

    void _drawIndicator(uint32_t cursor_line_y) override {
        Tty::getInstance().addString(cursor_line_y, core_top_left_.col, indicator_,
                                     cs_.getColor(HighlightGroup::Indicator));
    }

    void _clearIndicator(uint32_t cursor_line_y) override {
        Tty::getInstance().addString(cursor_line_y, core_top_left_.col, std::string(indent_, ' '),
                                     cs_.getColor(HighlightGroup::Normal));
    }
private:
    std::string prompt_{ "> " };
    std::string indicator_{ "➤" };
    uint32_t cmdline_y_{ 1 };
    Point line_info_;   // line_info_.col is the end position of the string if on the cmdline,
                        // otherwise, it is the beginning position of the string
    uint32_t result_size_{ 0 };
    uint32_t total_size_{ 0 };
    uint32_t cursor_pos_{ 0 };
    static constexpr uint32_t right_boundary_{ 50 };

    std::atomic<uint32_t> flag_col_{ 1 };
    std::atomic<bool> show_flag_{ true };
};

class PreviewWindow : public Window
{
public:
    PreviewWindow(const Point& tl, const Point& br, const Colorscheme& cs): Window(tl, br, cs) {
        is_reverse_ = false;
        indent_ = 0;
    }

protected:

};

class Cleanup final : public Singleton<Cleanup>
{
    friend Singleton<Cleanup>;
public:
    ~Cleanup();
    void doWork(bool once=true);
    void saveCursorPosition(const Point& orig_cursor_pos) {
        orig_cursor_pos_ = orig_cursor_pos;
    }

    void setTopBorder() {
        top_border_ = true;
    }

    void saveCoreHeight(uint32_t height) {
        core_height_ = height;
    }

    void setFullScreen() {
        is_full_screen_ = true;
    }

private:
    bool done_{ false };
    bool is_full_screen_{ false };
    bool top_border_{ false };
    Point orig_cursor_pos_{ 0, 0 };
    uint32_t core_height_{ 1 };
};

class Tui final
{
public:
    Tui();
    ~Tui();

    void init(bool resume);

    void setMargin(Point& tl, Point& br, uint32_t height);

    template<typename... Args>
    void setMainWindow(Args&&... args) {
        p_main_win_.reset(new MainWindow(std::forward<Args>(args)...));
    }

    template<typename... Args>
    void setPreviewWindow(Args&&... args) {
        p_preview_win_.reset(new PreviewWindow(std::forward<Args>(args)...));
    }

    template <typename T>
    struct WindowType{};

    const std::unique_ptr<MainWindow>& getWindow(WindowType<MainWindow>) const noexcept {
        return p_main_win_;
    }

    const std::unique_ptr<PreviewWindow>& getWindow(WindowType<PreviewWindow>) const noexcept {
        return p_preview_win_;
    }

    Tty& getTty() const noexcept {
        return tty_;
    }

    const std::string& getColor(HighlightGroup group) const noexcept {
        return cs_.getColor(group);
    }

    template <typename T>
    void setBuffer(Generator&& generator) {
        auto& w = getWindow(WindowType<T>());
        if ( w ) {
            w->setBuffer(std::move(generator));
        }
    }

    template <typename T>
    void setBuffer() {
        auto& w = getWindow(WindowType<T>());
        if ( w ) {
            w->setBuffer();
        }
    }

    template <typename T>
    void scrollUp() {
        auto& w = getWindow(WindowType<T>());
        if ( w ) {
            w->scrollUp();
        }
    }

    template <typename T>
    void scrollDown() {
        auto& w = getWindow(WindowType<T>());
        if ( w ) {
            w->scrollDown();
        }
    }

    template <typename T>
    void pageUp() {
        auto& w = getWindow(WindowType<T>());
        if ( w ) {
            w->pageUp();
        }
    }

    template <typename T>
    void pageDown() {
        auto& w = getWindow(WindowType<T>());
        if ( w ) {
            w->pageDown();
        }
    }

    void singleClick(const Point& yx) {
        if ( p_main_win_->inCoreWindow(yx) ) {
            p_main_win_->singleClick(yx);
        }
    }

    void wheelDown(const Point& yx) {
        if ( p_main_win_->inWindow(yx) ) {
            p_main_win_->scrollDown();
        }
    }

    void wheelUp(const Point& yx) {
        if ( p_main_win_->inWindow(yx) ) {
            p_main_win_->scrollUp();
        }
    }

    template <typename T>
    uint32_t getCoreHeight() const noexcept {
        auto& w = getWindow(WindowType<T>());
        if ( w ) {
            return w->getCoreHeight();
        }
        else {
            return 0;
        }
    }

    template <typename T>
    uint32_t getCoreWidth() const noexcept {
        auto& w = getWindow(WindowType<T>());
        if ( w ) {
            return w->getCoreWidth();
        }
        else {
            return 0;
        }
    }

    void updateCmdline(const std::string& pattern, uint32_t cursor_pos) {
        p_main_win_->updateCmdline(pattern, cursor_pos);
    }

    void updateCmdline(const std::string& pattern) {
        p_main_win_->updateCmdline(pattern);
    }

    void updateLineInfo(uint32_t result_size, uint32_t total_size) {
        p_main_win_->updateLineInfo(result_size, total_size);
    }

    void updateLineInfo() {
        p_main_win_->updateLineInfo();
    }

    void showFlag(bool show) {
        p_main_win_->showFlag(show);
    }

    void redrawPrompt(bool normal_mode) {
        p_main_win_->redrawPrompt(normal_mode);
    }

    void drawBorder() const {
        p_main_win_->drawBorder();
    }

    void setAccept() {
        accept_ = true;
    }

private:
    Tty& tty_;
    const ConfigManager& cfg_;
    Cleanup& cleanup_;
    Colorscheme cs_;
    std::unique_ptr<MainWindow> p_main_win_;
    std::unique_ptr<PreviewWindow> p_preview_win_;
    bool accept_{ false };

};

} // end namespace leaf
