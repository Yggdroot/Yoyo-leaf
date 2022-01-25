_Pragma("once");

#include <string>
#include <memory>
#include <array>
#include <bitset>
#include <type_traits>
#include "utils.h"

namespace leaf
{

enum class Attribute
{
    Normal        = 0,
    Bold          = 1,
    Dim           = 2,
    Italic        = 3,
    Underline     = 4,
    SlowBlink     = 5,
    RapidBlink    = 6,
    Reverse       = 7,
    Conceal       = 8,
    Strikethrough = 9,

    MaxAttrNum
};

enum class HighlightGroup
{
    Normal,
    Match0,
    Match1,
    Match2,
    Match3,
    Match4,
    Prompt,
    CursorLine,
    Indicator,
    LineInfo,
    Flag,
    NormalMode,
    Border,

    MaxGroupNum
};

enum class ColorPriority {
    Normal,
    High
};

class Color8
{
public:
    Color8() = default;
    Color8(const std::string& fg, const std::string& bg,
           Attribute attr=Attribute::Normal,
           ColorPriority priority=ColorPriority::Normal) {
        if ( fg.empty() && bg.empty() && attr == Attribute::Normal) {
            return;
        }

        std::string attribute;
        if ( attr != Attribute::Normal ) {
            attribute = ";" + std::to_string(static_cast<uint32_t>(attr));
        }

        auto str = utils::strFormat<16>("%s%s%s%s%s%s",
                                         priority == ColorPriority::Normal ? "" : "0",
                                         attribute.c_str(),
                                         fg.empty() ? "" : ";",
                                         fg.c_str(),
                                         bg.empty() ? "" : ";",
                                         bg.c_str());

        auto p_str = str.c_str();
        esc_code_ = utils::strFormat<16>("\033[%sm", *p_str == ';' ? p_str + 1 : p_str);
    }

    Color8(const std::string& fg, const std::string& bg,
           const std::bitset<static_cast<uint32_t>(Attribute::MaxAttrNum)>& attr,
           ColorPriority priority=ColorPriority::Normal) {
        std::string attributes;
        uint32_t max_num = static_cast<uint32_t>(Attribute::MaxAttrNum);
        for ( uint32_t i = 1; i < max_num; ++i ) {
            if ( attr.test(i) ) {
                attributes += ";" + std::to_string(i);
            }
        }

        if ( fg.empty() && bg.empty() && attributes.empty() ) {
            return;
        }

        auto str = utils::strFormat<64>("%s%s%s%s%s%s",
                                         priority == ColorPriority::Normal ? "" : "0",
                                         attributes.c_str(),
                                         fg.empty() ? "" : ";",
                                         fg.c_str(),
                                         bg.empty() ? "" : ";",
                                         bg.c_str());

        auto p_str = str.c_str();
        esc_code_ = utils::strFormat<64>("\033[%sm", *p_str == ';' ? p_str + 1 : p_str);
    }

    const std::string& getColor() const noexcept {
        return esc_code_;
    }

private:
    std::string esc_code_{ "\033[0m" };
};

class Color256
{
public:
    Color256() = default;
    // fg and bg is 0-255
    Color256(const std::string& fg, const std::string& bg,
             Attribute attr=Attribute::Normal,
             ColorPriority priority=ColorPriority::Normal) {
        if ( fg.empty() && bg.empty() && attr == Attribute::Normal) {
            return;
        }

        std::string attribute;
        if ( attr != Attribute::Normal ) {
            attribute = ";" + std::to_string(static_cast<uint32_t>(attr));
        }

        auto str = utils::strFormat<32>("%s%s%s%s%s%s",
                                         priority == ColorPriority::Normal ? "" : "0",
                                         attribute.c_str(),
                                         fg.empty() ? "" : ";38;5;",
                                         fg.c_str(),
                                         bg.empty() ? "" : ";48;5;",
                                         bg.c_str());

        auto p_str = str.c_str();
        esc_code_ = utils::strFormat<32>("\033[%sm", *p_str == ';' ? p_str + 1 : p_str);
    }

    Color256(const std::string& fg, const std::string& bg,
             const std::bitset<static_cast<uint32_t>(Attribute::MaxAttrNum)>& attr,
             ColorPriority priority=ColorPriority::Normal) {
        std::string attributes;
        uint32_t max_num = static_cast<uint32_t>(Attribute::MaxAttrNum);
        for ( uint32_t i = 1; i < max_num; ++i ) {
            if ( attr.test(i) ) {
                attributes += ";" + std::to_string(i);
            }
        }

        if ( fg.empty() && bg.empty() && attributes.empty() ) {
            return;
        }

        auto str = utils::strFormat<64>("%s%s%s%s%s%s",
                                         priority == ColorPriority::Normal ? "" : "0",
                                         attributes.c_str(),
                                         fg.empty() ? "" : ";38;5;",
                                         fg.c_str(),
                                         bg.empty() ? "" : ";48;5;",
                                         bg.c_str());

        auto p_str = str.c_str();
        esc_code_ = utils::strFormat<64>("\033[%sm", *p_str == ';' ? p_str + 1 : p_str);
    }

    const std::string& getColor() const noexcept {
        return esc_code_;
    }

private:
    std::string esc_code_{ "\033[0m" };
};


class Color24Bit
{
public:
    Color24Bit() = default;
    // fg and bg is Hex color code, e.g., #FF7F50, which represents RGB(255,127,80)
    Color24Bit(const std::string& fg, const std::string& bg,
               Attribute attr=Attribute::Normal,
               ColorPriority priority=ColorPriority::Normal) {
        if ( fg.empty() && bg.empty() && attr == Attribute::Normal) {
            return;
        }

        std::string attribute;
        if ( attr != Attribute::Normal ) {
            attribute = ";" + std::to_string(static_cast<uint32_t>(attr));
        }

        auto str = utils::strFormat<64>("%s%s%s%s%s%s",
                                         priority == ColorPriority::Normal ? "" : "0",
                                         attribute.c_str(),
                                         fg.empty() ? "" : ";38;2;",
                                         hex2Rgb(fg).c_str(),
                                         bg.empty() ? "" : ";48;2;",
                                         hex2Rgb(bg).c_str());

        auto p_str = str.c_str();
        esc_code_ = utils::strFormat<64>("\033[%sm", *p_str == ';' ? p_str + 1 : p_str);
    }

    Color24Bit(const std::string& fg, const std::string& bg,
               const std::bitset<static_cast<uint32_t>(Attribute::MaxAttrNum)>& attr,
               ColorPriority priority=ColorPriority::Normal) {
        std::string attributes;
        uint32_t max_num = static_cast<uint32_t>(Attribute::MaxAttrNum);
        for ( uint32_t i = 1; i < max_num; ++i ) {
            if ( attr.test(i) ) {
                attributes += ";" + std::to_string(i);
            }
        }

        if ( fg.empty() && bg.empty() && attributes.empty() ) {
            return;
        }

        auto str = utils::strFormat<64>("%s%s%s%s%s%s",
                                         priority == ColorPriority::Normal ? "" : "0",
                                         attributes.c_str(),
                                         fg.empty() ? "" : ";38;2;",
                                         hex2Rgb(fg).c_str(),
                                         bg.empty() ? "" : ";48;2;",
                                         hex2Rgb(bg).c_str());

        auto p_str = str.c_str();
        esc_code_ = utils::strFormat<64>("\033[%sm", *p_str == ';' ? p_str + 1 : p_str);
    }

    const std::string& getColor() const noexcept {
        return esc_code_;
    }

    // e.g., hex is #FF7F50
    std::string hex2Rgb(const std::string& hex) {
        std::string rgb;
        if ( !hex.empty() ) {
            int hex_color = std::stoi(hex.substr(1), nullptr, 16);
            int r = (hex_color >> 16) & 0xFF;
            int g = (hex_color >> 8) & 0xFF;
            int b = hex_color & 0xFF;
            rgb = utils::strFormat<16>("%d;%d;%d", r, g, b);
        }
        return rgb;
    }
private:
    std::string esc_code_{ "\033[0m" };
};

class Color final
{
    class ColorInterface
    {
    public:
        virtual ~ColorInterface() = default;
        virtual const std::string& getColor() const = 0;
    };

    struct ColorImplTag {};

    template <typename T>
    class ColorImpl final: public ColorInterface
    {
    public:

        template <typename U>
        explicit ColorImpl(ColorImplTag, U&& color): color_(std::forward<U>(color)) {
            static_assert(std::is_same<T, U>::value, "Must be the same type!");
        }

        const std::string& getColor() const override {
            return color_.getColor();
        }

    private:
        T color_;
    };

public:
    Color() = default;

    template <typename T,
              typename=std::enable_if_t<!std::is_same<std::decay_t<T>, Color>::value>
             >
    explicit Color(T&& c): p_color_(new ColorImpl<T>(ColorImplTag(), std::forward<T>(c))) {}

    const std::string& getColor() const noexcept {
        return p_color_ ? p_color_->getColor() : getDefaultColor();
    }

    template <typename T>
    void setColor(T&& color) {
        p_color_.reset(new ColorImpl<T>(ColorImplTag(), std::forward<T>(color)));
    }
private:
    const std::string& getDefaultColor() const noexcept {
        static std::string color{ "\033[0m" };
        return color;
    }

private:
    std::unique_ptr<ColorInterface> p_color_;
};


using ColorArray = std::array<Color, static_cast<uint32_t>(HighlightGroup::MaxGroupNum)>;

class Colorscheme
{
public:
    Colorscheme() {
        colors_[static_cast<uint32_t>(HighlightGroup::Prompt)].setColor(Color256("1", "", Attribute::Bold));
        colors_[static_cast<uint32_t>(HighlightGroup::CursorLine)].setColor(Color256("228", "", Attribute::Bold));
        colors_[static_cast<uint32_t>(HighlightGroup::Indicator)].setColor(Color256("5", "", Attribute::Bold));
        colors_[static_cast<uint32_t>(HighlightGroup::Match0)].setColor(Color256("155", "", Attribute::Bold));
        colors_[static_cast<uint32_t>(HighlightGroup::LineInfo)].setColor(Color256("254", "59"));
        colors_[static_cast<uint32_t>(HighlightGroup::Flag)].setColor(Color256("218", ""));
        colors_[static_cast<uint32_t>(HighlightGroup::NormalMode)].setColor(Color256("1", "", Attribute::Reverse));
        colors_[static_cast<uint32_t>(HighlightGroup::Border)].setColor(Color256("246", ""));
        //colors_[static_cast<uint32_t>(HighlightGroup::Normal)].setColor(Color256("", "", Attribute::Normal, ColorPriority::High));
    }

    const std::string& getColor(HighlightGroup group) const noexcept {
        return colors_[static_cast<uint32_t>(group)].getColor();
    }

    template <typename T>
    void setColor(HighlightGroup group, T&& color) {
        colors_[static_cast<uint32_t>(group)].setColor(std::forward<T>(color));
    }

    const ColorArray& getColorArray() const noexcept {
        return colors_;
    }
private:
    ColorArray colors_;
};


} // end namespace leaf
