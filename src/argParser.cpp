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
    std::string help("usage: yy [-h]");
    help.reserve(1024);
    std::vector<std::map<std::string, const Argument*>> arg_groups(static_cast<uint32_t>(ArgCategory::MaxNum));

    for ( auto& arg : args_ ) {
        auto category = static_cast<uint32_t>(arg.second.category);
        arg_groups[category].emplace(arg.first, &arg.second);

        help += " [";
        if ( !arg.second.alias.empty() ) {
            help += arg.second.alias;
        }
        else {
            help += arg.first;
        }
        if ( !arg.second.metavar.empty() ) {
            help += "=" + arg.second.metavar;
        }
        help += "]";
    }

    help += "\r\n";

    for ( auto& group : arg_groups ) {
        bool header = false;
        for ( auto& g : group ) {
            auto& arg = *g.second;
            if ( header == false ) {
                header = true;
                help += "\r\n  " + category_name_[arg.category] + "\r\n";
            }

            std::string arg_name = g.first;
            if ( !arg.alias.empty() ) {
                arg_name = arg.alias + ", " + arg_name;
            }

            auto name_column = utils::strFormat<32>("%4c%-20s", ' ', arg_name.c_str());
            help += name_column;

            uint32_t width = 56;
            uint32_t start = 0;
            uint32_t end = start + width;
            while ( end < arg.help.length() ) {
                if ( start > 0 ) {
                    help += std::string(name_column.length(), ' ');
                }

                while ( end > start && arg.help[end] != ' ' && arg.help[end] != '\t' ) {
                    --end;
                }

                if ( end == start ) {
                    end = start + width;
                }

                help += arg.help.substr(start, end - start) + "\r\n";

                start = end;
                while ( start < arg.help.length() && (arg.help[start] == ' ' || arg.help[start] == '\t') ) {
                    ++start;
                }
                end = start + width;
            }

            if ( start < arg.help.length() ) {
                if ( start > 0 ) {
                    help += std::string(name_column.length(), ' ');
                }
                help += arg.help.substr(start) + "\r\n";
            }
        }
    }

    printf("%s\r\nalias: leaf\r\n", help.c_str());
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
                printf("unknown option: %s\r\n", key.c_str());
                std::exit(EXIT_FAILURE);
            }
            else {
                iter = args_.find(it->second);
                if ( iter == args_.end() ) {
                    printf("unknown option: %s\r\n", key.c_str());
                    std::exit(EXIT_FAILURE);
                }
            }
        }

        auto& argument = iter->second;
        if ( eq != nullptr ) {
            if ( argument.nargs == "0" ) {
                printf("unknown option: %s\r\n", argv[i]);
                std::exit(EXIT_FAILURE);
            }
            else if ( val_list[0].empty() ) {
                printf("no value specified for: %s\r\n", argv[i]);
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
                    printf("no value specified for: %s\r\n", key.c_str());
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
                    printf("not enough value specified for: %s\r\n", key.c_str());
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
                    printf("invalid value: %s\r\n", val_list[0].c_str());
                    std::exit(EXIT_FAILURE);
                }
            }
            else {
                try {
                    value = stoi(val_list[0]);
                }
                catch(...) {
                    printf("invalid value: %s\r\n", val_list[0].c_str());
                    std::exit(EXIT_FAILURE);
                }
            }

            SetConfigValue(cfg, Height, value);
            break;
        }
        case ConfigType::SortPreference:
        {
            if ( val_list[0] == "begin" ) {
                SetConfigValue(cfg, SortPreference, Preference::Begin);
            }
            else if ( val_list[0] == "end" ) {
                SetConfigValue(cfg, SortPreference, Preference::End);
            }
            else {
                printf("invalid value: %s for %s\r\n", val_list[0].c_str(), key.c_str());
                std::exit(EXIT_FAILURE);
            }
            break;
        }
        default:
            break;
        }
    }

}

} // end namespace leaf
