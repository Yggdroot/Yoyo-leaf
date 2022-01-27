#include <thread>
#include "tui.h"

namespace leaf
{

Window::Window(const Point& tl, const Point& br, const Colorscheme& cs)
    : cs_(cs)
{
    _init(tl, br);
}

void Window::_init(const Point& tl, const Point& br) {
    top_left_ = tl;
    bottom_right_ = br;
    core_top_left_ = top_left_;

    height_ = bottom_right_.line - top_left_.line + 1;
    width_ = bottom_right_.col - top_left_.col + 1;
    core_height_ = height_;
    core_width_ = width_;
    setBorder();
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

void Window::setBuffer() {
    uint32_t orig_cursorline_y = core_top_left_.line + cursor_line_ - first_line_;
    if ( is_reverse_ ) {
        orig_cursorline_y = core_top_left_.line - (cursor_line_ - first_line_);
    }

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
            line_info_.col = core_top_left_.col - 1 + right_boundary_;
        }
        int32_t info_visual_len = core_top_left_.col + core_width_ - (line_info_.col - info.length() + 1);
        if ( info_visual_len > 0 ) {
            tty.addString(line_info_.line, line_info_.col - info.length() + 1,
                          info.substr(0, info_visual_len),
                          cs_.getColor(HighlightGroup::LineInfo));
        }

        int32_t visual_len = core_top_left_.col + core_width_ - (line_info_.col - info_len + 1);
        if ( visual_len > 0 ) {
            tty.addString(line_info_.line, line_info_.col - info_len + 1,
                          std::string(info_len - info.length(), ' ').substr(0, visual_len));
        }

        uint32_t flag_len = 0;
        if ( show_flag_.load(std::memory_order_relaxed) ) {
            flag_len = 3;
            if ( line_info_.col + 1 < core_top_left_.col + core_width_ ) {
                tty.addString(line_info_.line, line_info_.col + 1, " ");
            }
        }
        int32_t space_len = core_top_left_.col + core_width_ - (line_info_.col + flag_len + 1);
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
    cursor_pos_ = cursor_pos;
    auto& tty = Tty::getInstance();

    if ( line_info_.line == cmdline_y_ ) {
        auto total = std::to_string(total_size_);
        auto info_len = total.length() * 2 + 1;
        auto info = std::to_string(result_size_) + "/" + total;
        auto first_len = prompt_.length() + pattern.length() + 2;
        std::string spaces;
        auto border_char_width = ConfigManager::getInstance().getBorderCharWidth();
        if ( border_char_width + first_len + info_len < right_boundary_ ) {
            line_info_.col = core_top_left_.col - 1 + right_boundary_;
            spaces = std::string(right_boundary_ - (first_len - 2) - info.length(), ' ');
        }
        else {
            line_info_.col = core_top_left_.col + first_len + info_len - 1;
            spaces = std::string(info_len - info.length() + 2, ' ');
        }

        uint32_t flag_len = 0;
        if ( show_flag_.load(std::memory_order_relaxed) ) {
            flag_col_.store(line_info_.col + 2, std::memory_order_relaxed);
            flag_len = 3;
            if ( line_info_.col + 1 < core_top_left_.col + core_width_ ) {
                tty.addString(line_info_.line, line_info_.col + 1, " ");
            }
        }

        int32_t info_visual_len = core_top_left_.col + core_width_ - (line_info_.col - info.length() + 1);
        if ( info_visual_len > 0 ) {
            tty.addString(line_info_.line, line_info_.col - info.length() + 1,
                          info.substr(0, info_visual_len),
                          cs_.getColor(HighlightGroup::LineInfo));
        }
        int32_t space_len = core_top_left_.col + core_width_ - (line_info_.col + flag_len + 1);
        if ( space_len > 0 ) {
            tty.addString(line_info_.line, line_info_.col + flag_len + 1, std::string(space_len, ' '));
        }

        int32_t visual_len = core_width_ - prompt_.length();
        tty.addString(cmdline_y_, core_top_left_.col + prompt_.length(),
                      (pattern + spaces).substr(0, visual_len));
        tty.addStringAndSave(cmdline_y_, core_top_left_.col + prompt_.length(),
                             pattern.substr(0, cursor_pos).substr(0, visual_len - 1));
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
    // this line can ensure the cursor is at the correct position
    tty.restoreCursorPosition();
}

/**
 * top, right, bottom, left, topleft, topright, botright, botleft corner
 *  Example: ['─', '│', '─', '│', '╭', '╮', '╯', '╰']
 *           ['─', '│', '─', '│', '┌', '┐', '┘', '└']
 *           ['━', '┃', '━', '┃', '┏', '┓', '┛', '┗']
 *           ['═', '║', '═', '║', '╔', '╗', '╝', '╚']
 */
void MainWindow::drawBorder() const {
    auto& border = ConfigManager::getInstance().getConfigValue<ConfigType::Border>();
    if ( border.empty() ) {
        return;
    }

    auto& border_chars = ConfigManager::getInstance().getConfigValue<ConfigType::BorderChars>();
    auto border_char_width = ConfigManager::getInstance().getBorderCharWidth();
    uint32_t count = width_ / border_char_width;

    // top
    if ( (border_mask_ & 0b0001) == 0b0001 ) {
        Cleanup::getInstance().setTopBorder();
        std::string border_str;
        border_str.reserve(border_chars[0].length() * count);
        for ( uint32_t i = 0; i < count; ++i ) {
            border_str.append(border_chars[0]);
        }

        Tty::getInstance().addString(top_left_.line, top_left_.col, border_str,
                                     cs_.getColor(HighlightGroup::Border));
    }

    // right
    if ( (border_mask_ & 0b0010) == 0b0010 ) {
        for ( uint32_t i = 0; i < height_; ++i ) {
            Tty::getInstance().addString(top_left_.line + i, bottom_right_.col - border_char_width + 1,
                                         border_chars[1], cs_.getColor(HighlightGroup::Border));
        }
    }

    // bottom
    if ( (border_mask_ & 0b0100) == 0b0100 ) {
        std::string border_str;
        border_str.reserve(border_chars[2].length() * count);
        for ( uint32_t i = 0; i < count; ++i ) {
            border_str.append(border_chars[2]);
        }

        Tty::getInstance().addString(bottom_right_.line, top_left_.col, border_str,
                                     cs_.getColor(HighlightGroup::Border));
    }

    // left
    if ( (border_mask_ & 0b1000) == 0b1000 ) {
        for ( uint32_t i = 0; i < height_; ++i ) {
            Tty::getInstance().addString(top_left_.line + i, top_left_.col, border_chars[3],
                                         cs_.getColor(HighlightGroup::Border));
        }
    }

    // top left corner
    if ( (border_mask_ & 0b1001) == 0b1001 ) {
        Tty::getInstance().addString(top_left_.line, top_left_.col, border_chars[4],
                                     cs_.getColor(HighlightGroup::Border));
    }

    // top right corner
    if ( (border_mask_ & 0b0011) == 0b0011 ) {
        Tty::getInstance().addString(top_left_.line, bottom_right_.col - border_char_width + 1,
                                     border_chars[5], cs_.getColor(HighlightGroup::Border));
    }

    // bottom right corner
    if ( (border_mask_ & 0b0110) == 0b0110 ) {
        Tty::getInstance().addString(bottom_right_.line, bottom_right_.col - border_char_width + 1,
                                     border_chars[6], cs_.getColor(HighlightGroup::Border));
    }

    // bottom left corner
    if ( (border_mask_ & 0b1100) == 0b1100 ) {
        Tty::getInstance().addString(bottom_right_.line, top_left_.col, border_chars[7],
                                     cs_.getColor(HighlightGroup::Border));
    }
}

void MainWindow::_setCmdline() {
    core_height_ -= 1;
    if ( is_reverse_ ) {
        cmdline_y_ = core_top_left_.line;
        if ( core_width_ < right_boundary_ + 3 ) {
            core_height_ -= 1;
            line_info_.line = cmdline_y_ - 1;
            line_info_.col = core_top_left_.col + 2;
            flag_col_.store(core_top_left_.col, std::memory_order_relaxed);
        }
        else {
            line_info_.line = cmdline_y_;
            line_info_.col = 0;
            flag_col_.store(core_top_left_.col - 1 + right_boundary_ + 2, std::memory_order_relaxed);
        }
        core_top_left_.line = line_info_.line - 1;
    }
    else {
        cmdline_y_ = core_top_left_.line;
        if ( core_width_ < right_boundary_ + 3 ) {
            core_height_ -= 1;
            line_info_.line = cmdline_y_ + 1;
            line_info_.col = core_top_left_.col + 2;
            flag_col_.store(core_top_left_.col, std::memory_order_relaxed);
        }
        else {
            line_info_.line = cmdline_y_;
            line_info_.col = 0;
            flag_col_.store(core_top_left_.col - 1 + right_boundary_ + 2, std::memory_order_relaxed);
        }
        core_top_left_.line = line_info_.line + 1;
    }
}

Tui::Tui()
    :tty_(Tty::getInstance()),
    cfg_(ConfigManager::getInstance()),
    cleanup_(Cleanup::getInstance())
{
    init(false);
}

void Tui::init(bool resume) {
    uint32_t win_height = 0;
    uint32_t win_width = 0;
    if ( !resume ) {
        tty_.getWindowSize(win_height, win_width);
    }
    else {
        tty_.getWindowSize2(win_height, win_width);
    }

    uint32_t line = 0;
    uint32_t col = 0;
    if ( !resume ) {
        tty_.getCursorPosition(line, col);
        auto& border = ConfigManager::getInstance().getConfigValue<ConfigType::Border>();
        if ( !border.empty() ) {
            auto& border_chars = ConfigManager::getInstance().getConfigValue<ConfigType::BorderChars>();
            tty_.addString(line, col, border_chars[0]);
            tty_.flush();
            uint32_t new_line = 0;
            uint32_t new_col = 0;
            tty_.getCursorPosition(new_line, new_col);
            ConfigManager::getInstance().setBorderCharWidth(new_col - col);
            tty_.clear(EraseMode::ToLineBegin);
        }
    }
    else {
        tty_.getCursorPosition2(line, col);
    }

    Point top_left(1, 1);
    Point bottom_right(win_height, win_width);

    auto height =  cfg_.getConfigValue<ConfigType::Height>();
    if ( height == 0 || height >= win_height ) {
        height = win_height;
        Point orig_cursor_pos;
        orig_cursor_pos.line = line;
        orig_cursor_pos.col = col;
        cleanup_.saveCursorPosition(orig_cursor_pos);
        cleanup_.setFullScreen();
        tty_.enableAlternativeBuffer();
        tty_.enableMouse();
        tty_.disableAutoWrap();
        tty_.flush();
    }
    else {
        uint32_t min_height = 3;  // minimum height is 3
        auto& border = ConfigManager::getInstance().getConfigValue<ConfigType::Border>();
        if ( border.find("T") != std::string::npos ) {
            ++min_height;
        }
        if ( border.find("B") != std::string::npos ) {
            ++min_height;
        }

        height = std::max(height, min_height);
        tty_.enableMouse();
        tty_.disableAutoWrap();
        if ( win_height - line < height ) {
            auto delta_height = height - (win_height - line);
            tty_.scrollUp(delta_height - 1);
            tty_.moveCursor(CursorDirection::Up, delta_height - 1);
            tty_.flush();
            line = win_height - height + 1;
        }
        else {
            tty_.flush();
            bottom_right.line = line + height - 1;
        }

        top_left.line = line;
        top_left.col = col;
    }

    setMargin(top_left, bottom_right, height);
    win_width = bottom_right.col - top_left.col + 1;
    auto& border = ConfigManager::getInstance().getConfigValue<ConfigType::Border>();
    if ( border.find("T") != std::string::npos || border.find("B") != std::string::npos ) {
        bottom_right.col -= win_width % ConfigManager::getInstance().getBorderCharWidth();
    }

    if ( !resume ) {
        setMainWindow(top_left, bottom_right, cs_);
        cleanup_.saveCoreHeight(p_main_win_->getCoreHeight());
        p_main_win_->display();
    }
    else {
        p_main_win_->reset(top_left, bottom_right);
    }
}

void Tui::setMargin(Point& tl, Point& br, uint32_t height) {
    auto& margin = ConfigManager::getInstance().getConfigValue<ConfigType::Margin>();
    uint32_t min_height = 5;
    if ( height <= min_height ) {
        margin[0] = 0;
        margin[2] = 0;
    }
    else if ( margin[0] + margin[2] + min_height > height ) {
        margin[0] = (height - min_height) >> 1;
        margin[2] = height - min_height - margin[0];
    }

    auto width = br.col - tl.col + 1;
    uint32_t min_width = 16;
    if ( width <= min_width ) {
        margin[1] = 0;
        margin[3] = 0;
    }
    else if ( margin[1] + margin[3] + min_width > width ) {
        margin[1] = (width - min_width) >> 1;
        margin[3] = width - min_width - margin[1];
    }

    tl.line += margin[0];
    tl.col += margin[3];
    br.line -= margin[2];
    br.col -= margin[1];
}

Tui::~Tui() {
    cleanup_.doWork(true);

    if ( accept_ ) {
        p_main_win_->printAcceptedStrings();
    }
}

Cleanup::~Cleanup() {
    if ( !done_ ) {
        // reduce the possibility that cmdline thread is still running
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        doWork(true);
    }
}

void Cleanup::doWork(bool once) {
    if ( once && !done_ ) {
        done_ = true;
    }

    auto& cfg = ConfigManager::getInstance();
    auto& tty = Tty::getInstance();
    tty.enableAutoWrap();
    tty.disableMouse();
    tty.showCursor();
    if ( is_full_screen_ ) {
        tty.clear(EraseMode::EntireScreen);
        tty.disableAlternativeBuffer();
        tty.moveCursorTo(orig_cursor_pos_.line, orig_cursor_pos_.col);
        tty.flush();
    }
    else {
        if ( cfg.getConfigValue<ConfigType::Reverse>() && core_height_ > 0 ) {
            tty.moveCursor(CursorDirection::Up, core_height_);
        }
        if ( top_border_ ) {
            tty.moveCursor(CursorDirection::Up, 1);
        }
        auto top_margin = cfg.getConfigValue<ConfigType::Margin>()[0];
        if ( top_margin ) {
            tty.moveCursor(CursorDirection::Up, top_margin);
        }
        tty.moveCursor(CursorDirection::Left, 1024);   // move to column 1
        tty.clear(EraseMode::ToScreenEnd);
        tty.flush();
    }

    tty.restoreOrigTerminal();
}

} // end namespace leaf
