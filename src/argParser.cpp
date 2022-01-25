#include <cstring>
#include <vector>
#include <map>
#include <stdio.h>
#include "argParser.h"
#include "tty.h"
#include "utils.h"


namespace leaf
{

ArgumentParser::ArgumentParser()
    : alias_{
        { "-r", "--reverse" }
    }, category_name_ {
        { ArgCategory::Layout, "Layout" },
        { ArgCategory::Search, "Search" },
    }, args_{
        { "--reverse",
            {
                ArgCategory::Layout,
                "-r",
                ConfigType::Reverse,
                "0",
                "",
                "Display from the bottom of the screen to top.",
            }
        },
        { "--height",
            {
                ArgCategory::Layout,
                "",
                ConfigType::Height,
                "1",
                "N[%]",
                "Display window with the given height N[%] instead of using fullscreen."
            }
        },
        { "--border",
            {
                ArgCategory::Layout,
                "",
                ConfigType::Border,
                "?",
                "BORDER[:STYLE]",
                "Display the T(top), R(right), B(bottom), L(left) border. BORDER is a string "
                "combined by 'T', 'R', 'B' or 'L'. If BORDER is not specified, display borders all around. "
                "STYLE is 1, 2, 3 and 4 which denotes \"[─,│,─,│,╭,╮,╯,╰]\", \"[─,│,─,│,┌,┐,┘,└]\", "
                "\"[━,┃,━,┃,┏,┓,┛,┗]\", \"[═,║,═,║,╔,╗,╝,╚]\" respectively."
            }
        },
        { "--border-chars",
            {
                ArgCategory::Layout,
                "",
                ConfigType::BorderChars,
                "1",
                "CHARS",
                "Specify the character to use for the top/right/bottom/left border, "
                "followed by the character to use for the topleft/topright/botright/botleft corner. "
                "Default value is \"[─,│,─,│,╭,╮,╯,╰]\""
            }
        },
        { "--sort-preference",
            {
                ArgCategory::Search,
                "",
                ConfigType::SortPreference,
                "1",
                "PREFERENCE",
                "Specify the sort preference to apply, value can be [begin|end]. (default: end)"
            }
        },
    }
{

}

void ArgumentParser::printHelp() {
    std::string help("usage: yy [options]\n");
    help.reserve(1024);
    std::vector<std::map<std::string, const Argument*>> arg_groups(static_cast<uint32_t>(ArgCategory::MaxNum));

    for ( auto& arg : args_ ) {
        auto category = static_cast<uint32_t>(arg.second.category);
        arg_groups[category].emplace(arg.first, &arg.second);
    }

    uint32_t name_column_width = 32;
    for ( auto& group : arg_groups ) {
        bool header = false;
        for ( auto& g : group ) {
            auto& arg = *g.second;
            if ( header == false ) {
                header = true;
                help += "\n  " + category_name_[arg.category] + "\n";
            }

            std::string arg_name = g.first;
            if ( !arg.alias.empty() ) {
                arg_name = arg.alias + ", " + arg_name;
            }

            if ( arg.nargs == "1" ) {
                arg_name += "=<" + arg.metavar + ">";
            }
            else if ( arg.nargs == "?" ) {
                arg_name += " [" + arg.metavar + "]";
            }
            else if ( arg.nargs == "*" ) {
                arg_name += " [" + arg.metavar + "]...";
            }
            else if ( arg.nargs == "+" ) {
                arg_name += " <" + arg.metavar + ">...";
            }

            auto name_column = utils::strFormat<64>("%4c%-28s", ' ', arg_name.c_str());
            help += name_column;
            if ( name_column.length() >= name_column_width ) {
                help += "\n" + std::string(name_column_width, ' ');
            }

            uint32_t width = 64;
            uint32_t start = 0;
            uint32_t end = start + width;
            while ( end < arg.help.length() ) {
                if ( start > 0 ) {
                    help += std::string(name_column_width, ' ');
                }

                while ( end > start && arg.help[end] != ' ' && arg.help[end] != '\t' ) {
                    --end;
                }

                if ( end == start ) {
                    end = start + width;
                }

                help += arg.help.substr(start, end - start) + "\n";

                start = end;
                while ( start < arg.help.length() && (arg.help[start] == ' ' || arg.help[start] == '\t') ) {
                    ++start;
                }
                end = start + width;
            }

            if ( start < arg.help.length() ) {
                if ( start > 0 ) {
                    help += std::string(name_column_width, ' ');
                }
                help += arg.help.substr(start) + "\n";
            }
        }
    }

    printf("%s\nalias: leaf\n", help.c_str());
}

void ArgumentParser::parseArgs(int argc, char* argv[], std::vector<std::unique_ptr<ConfigBase>>& cfg) {
    for ( int i = 1; i < argc; ++i ) {
        if ( strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ) {
            printHelp();
            std::exit(0);
        }
    }

    for ( int i = 1; i < argc; ++i ) {
        auto key = std::string(argv[i]);
        auto eq = strchr(argv[i], '=');
        std::vector<std::string> val_list;
        if ( eq != nullptr ) {
            key = key.substr(0, eq - argv[i]);
            val_list.emplace_back(eq + 1);
        }

        auto iter = args_.find(key);
        if ( iter == args_.end() ) {
            auto it = alias_.find(key);
            if ( it == alias_.end() ) {
                printf("unknown option: %s\n", key.c_str());
                std::exit(EXIT_FAILURE);
            }
            else {
                iter = args_.find(it->second);
                if ( iter == args_.end() ) {
                    printf("unknown option: %s\n", key.c_str());
                    std::exit(EXIT_FAILURE);
                }
            }
        }

        auto& argument = iter->second;
        if ( eq != nullptr ) {
            if ( argument.nargs == "0" ) {
                printf("unknown option: %s\n", argv[i]);
                std::exit(EXIT_FAILURE);
            }
            else if ( val_list[0].empty() ) {
                printf("no value specified for: %s\n", argv[i]);
                std::exit(EXIT_FAILURE);
            }
        }
        else {
            if ( argument.nargs == "*" || argument.nargs == "+" ) {
                int j = i + 1;
                for ( ; j < argc; ++j ) {
                    if ( argv[j][0] != '-' ) {
                        val_list.emplace_back(argv[j]);
                    }
                    else {
                        break;
                    }
                }
                i = j - 1;

                if ( argument.nargs == "+" && val_list.empty() ) {
                    printf("no value specified for: %s\n", key.c_str());
                    std::exit(EXIT_FAILURE);
                }
            }
            else if ( argument.nargs == "?" ) {
                int j = i + 1;
                if ( j < argc && argv[j][0] != '-' ) {
                    val_list.emplace_back(argv[j]);
                    i = j;
                }
            }
            else {
                uint32_t n = stoi(argument.nargs);
                int j = i + 1;
                for ( ; j < argc && j <= static_cast<int>(i + n); ++j ) {
                    if ( argv[j][0] != '-' ) {
                        val_list.emplace_back(argv[j]);
                    }
                    else {
                        break;
                    }
                }
                i = j - 1;

                if ( val_list.size() < n ) {
                    printf("not enough value specified for: %s\n", key.c_str());
                    std::exit(EXIT_FAILURE);
                }
            }
        }

        switch ( argument.cfg_type )
        {
        case ConfigType::Reverse:
            SetConfigValue(cfg, Reverse, true);
            break;
        case ConfigType::Height:
        {
            uint32_t value = 0;
            auto len = val_list[0].size();
            if ( val_list[0][len - 1] == '%' ) {
                uint32_t win_height = 0;
                uint32_t win_width = 0;
                Tty::getInstance().getWindowSize(win_height, win_width);
                try {
                    value = win_height * stoi(val_list[0].substr(0, len - 1)) / 100;
                }
                catch(...) {
                    printf("invalid value: %s\n", val_list[0].c_str());
                    std::exit(EXIT_FAILURE);
                }
            }
            else {
                try {
                    value = stoi(val_list[0]);
                }
                catch(...) {
                    printf("invalid value: %s\n", val_list[0].c_str());
                    std::exit(EXIT_FAILURE);
                }
            }

            SetConfigValue(cfg, Height, value);
            break;
        }
        case ConfigType::SortPreference:
            if ( val_list[0] == "begin" ) {
                SetConfigValue(cfg, SortPreference, Preference::Begin);
            }
            else if ( val_list[0] == "end" ) {
                SetConfigValue(cfg, SortPreference, Preference::End);
            }
            else {
                printf("invalid value: %s for %s\n", val_list[0].c_str(), key.c_str());
                std::exit(EXIT_FAILURE);
            }
            break;
        case ConfigType::Border:
            if ( !val_list.empty() ) {
                auto pos = val_list[0].find(':');
                auto val = val_list[0].substr(0, pos);
                if ( val.empty() ) {
                    SetConfigValue(cfg, Border, "TRBL");
                }
                else {
                    for ( auto c : val ) {
                        if ( c != 'T' && c != 'R' && c != 'B' && c != 'L' ) {
                            printf("invalid value: %s for %s\n", val_list[0].c_str(), key.c_str());
                            std::exit(EXIT_FAILURE);
                        }
                    }

                    SetConfigValue(cfg, Border, val);
                }

                if ( pos != std::string::npos ) {
                    auto style = val_list[0].substr(pos + 1);
                    if ( style.empty() || style == "1" ) {
                    }
                    else if ( style == "2" ) {
                        SetConfigValue(cfg, BorderChars,
                                       std::vector<std::string>({"─","│","─","│","┌","┐","┘","└"}));
                    }
                    else if ( style == "3" ) {
                        SetConfigValue(cfg, BorderChars,
                                       std::vector<std::string>({"━","┃","━","┃","┏","┓","┛","┗"}));
                    }
                    else if ( style == "4" ) {
                        SetConfigValue(cfg, BorderChars,
                                       std::vector<std::string>({"═","║","═","║","╔","╗","╝","╚"}));
                    }
                    else {
                        printf("invalid style: %s for %s\n", style.c_str(), key.c_str());
                        std::exit(EXIT_FAILURE);
                    }
                }
            }
            else {
                SetConfigValue(cfg, Border, "TRBL");
            }
            break;
        case ConfigType::BorderChars:
        {
            auto left = val_list[0].find('[');
            auto right = val_list[0].find(']');
            if ( left == std::string::npos || right == std::string::npos ) {
                printf("invalid value: %s for %s\n", val_list[0].c_str(), key.c_str());
                std::exit(EXIT_FAILURE);
            }
            std::vector<std::string> border_chars;
            for ( auto k = left + 1; k < right; ++k ) {
                while ( val_list[0][k] == ' ' ) {
                    ++k;
                }
                auto start = k;
                while ( k < right && val_list[0][k] != ' ' && val_list[0][k] != ',' ) {
                    ++k;
                }
                if ( k == start || (k - start > 1 && (val_list[0][start] & 0x80) == 0 ) ) {
                    printf("invalid value: %s for %s\n", val_list[0].c_str(), key.c_str());
                    std::exit(EXIT_FAILURE);
                }
                border_chars.emplace_back(val_list[0].substr(start, k - start));
                while ( val_list[0][k] == ' ' ) {
                    ++k;
                }

                if ( k < right && val_list[0][k] != ',' ) {
                    printf("invalid value: %s for %s\n", val_list[0].c_str(), key.c_str());
                    std::exit(EXIT_FAILURE);
                }
            }

            if ( border_chars.size() != 8 ) {
                printf("invalid value: %s for %s\n", val_list[0].c_str(), key.c_str());
                std::exit(EXIT_FAILURE);
            }

            SetConfigValue(cfg, BorderChars, std::move(border_chars));
            break;
        }
        default:
            break;
        }
    }

}

} // end namespace leaf
