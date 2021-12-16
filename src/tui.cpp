#include <thread>
#include "tui.h"

namespace leaf
{

Window::Window(const Point& tl, const Point& br, const Colorscheme& cs)
    :top_left_(tl), bottom_right_(br), core_top_left_(top_left_), cs_(cs)
{
    height_ = bottom_right_.line - top_left_.line + 1;
    width_ = bottom_right_.col - top_left_.col + 1;
    core_height_ = height_;
    core_width_ = width_;
    //if margin
    //    setMargin
    //if border
    //    setBorder
    if ( is_reverse_ ) {
        core_top_left_.line += core_height_ - 1;
    }
}

void Window::setBuffer(Generator&& generator) {
    generator_ = std::move(generator);
    buffer_ = generator_();
    uint32_t orig_cursorline_y = core_top_left_.line + cursor_line_ - first_line_;
    if ( is_reverse_ ) {
        orig_cursorline_y = core_top_left_.line - (cursor_line_ - first_line_);
    }
    first_line_ = 0;
    last_line_ = std::min(core_height_, static_cast<decltype(core_height_)>(buffer_.size()));
    cursor_line_ = 0;

    _render(orig_cursorline_y);
}

void Window::_scrollUp() {
    if ( cursor_line_ > first_line_ ) {
        _updateCursorline(cursor_line_ - 1);
    }
    else if ( first_line_ > 0 ) {
        uint32_t orig_cursorline_y = core_top_left_.line + cursor_line_ - first_line_;
        if ( is_reverse_ ) {
            orig_cursorline_y = core_top_left_.line - (cursor_line_ - first_line_);
        }
        --first_line_;
        if ( last_line_ > first_line_ + core_height_ ) {
            --last_line_;
        }
        --cursor_line_;
        _render(orig_cursorline_y);
    }
}

void Window::_scrollDown() {
    if ( cursor_line_ + 1 < last_line_ ) {
        _updateCursorline(cursor_line_ + 1);
        return;
    }

    if ( buffer_.size() == last_line_ ) {
        auto buffer = generator_();
        buffer_.insert(buffer_.end(), buffer.begin(), buffer.end());
    }

    if ( buffer_.size() > last_line_ ) {
        uint32_t orig_cursorline_y = core_top_left_.line + cursor_line_ - first_line_;
        if ( is_reverse_ ) {
            orig_cursorline_y = core_top_left_.line - (cursor_line_ - first_line_);
        }
        ++first_line_;
        ++last_line_;
        ++cursor_line_;
        _render(orig_cursorline_y);
    }
}

void Window::_pageUp() {
    if ( first_line_ == 0 ) {
        _updateCursorline(first_line_);
        return;
    }

    uint32_t orig_cursorline_y = core_top_left_.line + cursor_line_ - first_line_;
    if ( is_reverse_ ) {
        orig_cursorline_y = core_top_left_.line - (cursor_line_ - first_line_);
    }
    if ( first_line_ >= core_height_ - 1 ) {
        last_line_ = first_line_ + 1;
        first_line_ = last_line_ - core_height_;
    }
    else {
        first_line_ = 0;
        last_line_ = std::min(core_height_, static_cast<decltype(core_height_)>(buffer_.size()));
    }

    cursor_line_ = last_line_ - 1;
    _render(orig_cursorline_y);
}


void Window::_pageDown() {
    if ( buffer_.size() < last_line_ + core_height_ ) {
        auto buffer = generator_();
        buffer_.insert(buffer_.end(), buffer.begin(), buffer.end());
    }

    if ( last_line_ == buffer_.size() ) { // no next page
        _updateCursorline(last_line_ - 1);
        return;
    }

    uint32_t orig_cursorline_y = core_top_left_.line + cursor_line_ - first_line_;
    if ( is_reverse_ ) {
        orig_cursorline_y = core_top_left_.line - (cursor_line_ - first_line_);
    }
    first_line_ = last_line_ - 1;
    last_line_ = std::min(first_line_ + core_height_, static_cast<decltype(core_height_)>(buffer_.size()));

    cursor_line_ = first_line_;
    _render(orig_cursorline_y);
}


void Window::_render(uint32_t orig_cursorline_y) {
    auto& tty = Tty::getInstance();
    tty.hideCursor();
    if ( is_reverse_ ) {
        uint32_t new_cursorline_y = core_top_left_.line - (cursor_line_ - first_line_);
        uint32_t j = core_top_left_.line;
        for ( uint32_t i = first_line_; i < last_line_; ++i, --j ) {
            // avoid cursor line flickering
            if ( new_cursorline_y == orig_cursorline_y && new_cursorline_y == j ) {
                continue;
            }
            int32_t space_len = core_width_ - indent_ - buffer_[i].raw_str.len;
            space_len = std::max(space_len, 0);
            tty.addString(j, core_top_left_.col, std::string(indent_, ' ') + buffer_[i].str + std::string(space_len, ' '));
        }

        for ( ; j > core_top_left_.line - core_height_; --j ) {
            tty.addString(j, core_top_left_.col, std::string(core_width_, ' '));
        }
    }
    else {
        uint32_t new_cursorline_y = core_top_left_.line + cursor_line_ - first_line_;
        uint32_t j = core_top_left_.line;
        for ( uint32_t i = first_line_; i < last_line_; ++i, ++j ) {
            // avoid cursor line flickering
            if ( new_cursorline_y == orig_cursorline_y && new_cursorline_y == j ) {
                continue;
            }
            int32_t space_len = core_width_ - indent_ - buffer_[i].raw_str.len;
            space_len = std::max(space_len, 0);
            tty.addString(j, core_top_left_.col, std::string(indent_, ' ') + buffer_[i].str + std::string(space_len, ' '));
        }

        for ( ; j < core_top_left_.line + core_height_; ++j ) {
            tty.addString(j, core_top_left_.col, std::string(core_width_, ' '));
        }
    }

    if ( first_line_ < last_line_ ) {
        _renderCursorline(cursor_line_);
    }

    tty.restoreCursorPosition();
    tty.showCursor();
}

void Window::_renderCursorline(uint32_t cursor_line) {
    auto& tty = Tty::getInstance();
    Point point = _getCursorlinePosition(cursor_line);
    auto str_len = buffer_[cursor_line].raw_str.len;
    int32_t space_len = core_width_ - indent_ - str_len;
    if ( space_len > 0 ) {
        tty.addString(point.line, point.col + str_len, std::string(space_len, ' '));
    }

    const std::string reset_color("\033[0m");

    // no color
    if ( !utils::endswith(buffer_[cursor_line].str, reset_color) ) {
        tty.addString(point.line, point.col,
                      buffer_[cursor_line].str,
                      cs_.getColor(HighlightGroup::CursorLine));
    }
    else {
        auto& normal_color = cs_.getColor(HighlightGroup::Normal);
        auto& cursor_line_color = cs_.getColor(HighlightGroup::CursorLine);
        auto cursor_line_str = buffer_[cursor_line].str;
        auto pos = cursor_line_str.rfind(reset_color);
        if ( pos != std::string::npos ) {
            auto max_pos = pos;
            pos -= reset_color.length();
            auto normal_len = normal_color.length();
            while ( pos < max_pos
                    && (pos = cursor_line_str.rfind(normal_color, pos)) != std::string::npos ) {
                cursor_line_str.replace(pos, normal_len, cursor_line_color);
                pos -= normal_len;
            }

            tty.addString(point.line, point.col, cursor_line_str);
        }
    }

    _drawIndicator(point.line);
}


void Window::_updateCursorline(uint32_t new_cursorline) {
    if ( new_cursorline == cursor_line_ || new_cursorline >= buffer_.size() ) {
        return;
    }

    auto& tty = Tty::getInstance();
    Point point = _getCursorlinePosition(cursor_line_);
    _clearIndicator(point.line);
    tty.addString(point.line, point.col, buffer_[cursor_line_].str);

    _renderCursorline(new_cursorline);
    cursor_line_ = new_cursorline;

    tty.restoreCursorPosition();
}

void MainWindow::updateLineInfo(uint32_t result_size, uint32_t total_size) {
    result_size_ = result_size;
    total_size_ = total_size;

    auto total = std::to_string(total_size_);
    auto info_len = total.length() * 2 + 1;
    auto info = std::to_string(result_size_) + "/" + total;
    auto& tty = Tty::getInstance();
    if ( line_info_.line == cmdline_y_ ) {
        if ( line_info_.col == 0 ) {
            line_info_.col = right_boundary;
        }
        tty.addString(line_info_.line, line_info_.col - info.length() + 1, info,
                      cs_.getColor(HighlightGroup::LineInfo));
        tty.addString(line_info_.line, line_info_.col - info_len + 1,
                      std::string(info_len - info.length(), ' '));

        uint32_t flag_len = 0;
        if ( show_flag_.load(std::memory_order_relaxed) ) {
            flag_len = 3;
            tty.addString(line_info_.line, line_info_.col + 1, " ");
        }
        int32_t space_len = core_width_ - line_info_.col - flag_len;
        if ( space_len > 0 ) {
            tty.addString(line_info_.line, line_info_.col + flag_len + 1, std::string(space_len, ' '));
        }
    }
    else {
        tty.addString(line_info_.line, line_info_.col, info,
                      cs_.getColor(HighlightGroup::LineInfo));
        int32_t space_len = core_width_ - indent_ - info.length();
        if ( space_len > 0 ) {
            tty.addString(line_info_.line, line_info_.col + info.length(),
                          std::string(space_len, ' '));
        }
    }
    tty.restoreCursorPosition();
}

void MainWindow::updateCmdline(const std::string& pattern, uint32_t cursor_pos) {
    auto& tty = Tty::getInstance();
    if ( line_info_.line == cmdline_y_ ) {
        auto total = std::to_string(total_size_);
        auto info_len = total.length() * 2 + 1;
        auto info = std::to_string(result_size_) + "/" + total;
        auto first_len = prompt_.length() + pattern.length() + 2;
        std::string spaces;
        if ( first_len + info_len < right_boundary ) {
            line_info_.col = right_boundary;
            spaces = std::string(right_boundary - (first_len - 2) - info.length(), ' ');
        }
        else {
            line_info_.col = top_left_.col + first_len + info_len - 1;
            spaces = std::string(info_len - info.length() + 2, ' ');
        }

        uint32_t flag_len = 0;
        if ( show_flag_.load(std::memory_order_relaxed) ) {
            flag_col_.store(line_info_.col + 2, std::memory_order_relaxed);
            flag_len = 3;
            tty.addString(line_info_.line, line_info_.col + 1, " ");
        }

        tty.addString(line_info_.line, line_info_.col - info.length() + 1, info,
                      cs_.getColor(HighlightGroup::LineInfo));
        int32_t space_len = core_width_ - line_info_.col - flag_len;
        if ( space_len > 0 ) {
            tty.addString(line_info_.line, line_info_.col + flag_len + 1, std::string(space_len, ' '));
        }
        tty.addString(cmdline_y_, core_top_left_.col + prompt_.length(), pattern + spaces);
        tty.addStringAndSave(cmdline_y_, core_top_left_.col + prompt_.length(), pattern.substr(0, cursor_pos));
    }
    else {
        uint32_t available_width = core_width_ - prompt_.length() - 1;
        if ( static_cast<int32_t>(available_width) <= 0 ) {
            return;
        }
        if ( available_width <= pattern.length() ) {
            if ( cursor_pos >= available_width ) {
                tty.addStringAndSave(cmdline_y_, core_top_left_.col + prompt_.length(),
                                     pattern.substr(cursor_pos - available_width, available_width));
            }
            else {
                tty.addString(cmdline_y_, core_top_left_.col + prompt_.length(),
                              pattern.substr(0, available_width));
                tty.addStringAndSave(cmdline_y_, core_top_left_.col + prompt_.length(),
                                     pattern.substr(0, cursor_pos));
            }
        }
        else {
            tty.addString(cmdline_y_, core_top_left_.col + prompt_.length(),
                          pattern + std::string(available_width - pattern.length(), ' '));
            tty.addStringAndSave(cmdline_y_, core_top_left_.col + prompt_.length(),
                                 pattern.substr(0, cursor_pos));
        }
    }
    // if another thread restore cursor to previous position before saveCursorPosition,
    // this line can make sure the cursor is at the correct position
    tty.restoreCursorPosition();
}

void MainWindow::_setCmdline() {
    core_height_ -= 1;
    if ( is_reverse_ ) {
        cmdline_y_ = core_top_left_.line;
        if ( core_width_ < right_boundary + 3 ) {
            line_info_.line = cmdline_y_ - 1;
            line_info_.col = 3;
        }
        else {
            line_info_.line = cmdline_y_;
            line_info_.col = 0;
            flag_col_.store(right_boundary + 2, std::memory_order_relaxed);
        }
        core_top_left_.line = line_info_.line - 1;
    }
    else {
        cmdline_y_ = core_top_left_.line;
        if ( core_width_ < right_boundary + 3) {
            line_info_.line = cmdline_y_ + 1;
            line_info_.col = 3;
        }
        else {
            line_info_.line = cmdline_y_;
            line_info_.col = 0;
            flag_col_.store(right_boundary + 2, std::memory_order_relaxed);
        }
        core_top_left_.line = line_info_.line + 1;
    }
}

Tui::Tui()
    :tty_(Tty::getInstance()),
    cfg_(ConfigManager::getInstance()),
    cleanup_(Cleanup::getInstance())
{
    uint32_t win_height = 0;
    uint32_t win_width = 0;
    tty_.getWindowSize(win_height, win_width);

    Point top_left(1, 1);
    Point bottom_right(win_height, win_width);

    auto height =  cfg_.getConfigValue<ConfigType::Height>();
    if ( height == 0 ) {
        tty_.enableAlternativeBuffer();
        tty_.enableMouse();
        tty_.disableAutoWrap();
        tty_.flush();
    }
    else {
        height = std::max(height, 3u);  // minimum height is 3
        tty_.enableMouse();
        tty_.disableAutoWrap();
        uint32_t line = 0;
        uint32_t col = 0;
        tty_.getCursorPosition(line, col);
        if ( win_height - line < height ) {
            auto delta_height = height - (win_height - line);
            tty_.scrollUp(delta_height - 1);
            tty_.moveCursor(CursorDirection::Up, delta_height - 1);
            tty_.getCursorPosition(line, col);
        }
        else {
            bottom_right.line = line + height - 1;
        }

        top_left.line = line;
        top_left.col = col;
    }

    setMainWindow(top_left, bottom_right, cs_);
    p_main_win_->display();
}

Tui::~Tui() {
    cleanup_.doWork();

    if ( accept_ ) {
        p_main_win_->printAcceptedStrings();
    }
}

Cleanup::~Cleanup() {
    if ( !done_ ) {
        // reduce the possibility that cmdline thread is still running
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        doWork();
    }
}

void Cleanup::doWork() {
    if ( !done_ ) {
        done_ = true;
    }

    auto& cfg = ConfigManager::getInstance();
    auto& tty = Tty::getInstance();
    auto height =  cfg.getConfigValue<ConfigType::Height>();
    if ( height == 0 ) {
        tty.enableAutoWrap();
        tty.disableMouse();
        tty.disableAlternativeBuffer();
        tty.flush();
    }
    else {
        height = std::max(height, 3u);  // minimum height is 3
        tty.enableAutoWrap();
        tty.disableMouse();
        if ( cfg.getConfigValue<ConfigType::Reverse>() ) {
            tty.moveCursor(CursorDirection::Up, height - 1);
        }
        tty.moveCursor(CursorDirection::Left, 1024);   // move to column 1
        tty.clear(EraseMode::ToScreenEnd);
        tty.flush();
    }
}

} // end namespace leaf
