#include <cassert>
#include "argparse_formatter.hpp"
#include "argparse_util.hpp"

#include "argparse.hpp"

namespace argparse {
    constexpr size_t OPTION_HELP_SLACK = 2;
    std::string INDENT = "  ";
    std::string USAGE_PREFIX = "usage: ";

    std::string long_option_str(const Argument& argument);
    std::string short_option_str(const Argument& argument);
    std::string determine_metavar(const Argument& argument);
    /*
     * DefaultFormatter
     */
    DefaultFormatter::DefaultFormatter(size_t option_name_width, size_t total_width)
        : option_name_width_(option_name_width)
        , total_width_(total_width)
        {}

    void DefaultFormatter::set_parser(ArgumentParser* parser) {
        parser_ = parser;
    }

    std::string DefaultFormatter::format_usage() const {
        if (!parser_) throw ArgParseError("parser not initialized in help formatter");


        std::stringstream ss;
        ss << USAGE_PREFIX << parser_->prog();

        int num_unshown_options = 0;
        for (const auto& group : parser_->argument_groups()) {
            auto args = group.arguments();
            for(const auto& arg : args) {

                if(arg->show_in() != ShowIn::USAGE_AND_HELP) {
                    num_unshown_options++;
                    continue;
                }

                ss << " ";

                if (!arg->required()) {
                    ss << "[";
                }

                auto short_opt = short_option_str(*arg);
                if (!short_opt.empty()) {
                    ss << short_opt; 
                } else {
                    ss << long_option_str(*arg);
                }

                if (!arg->required()) {
                    ss << "]";
                }
            }
        }
        if (num_unshown_options > 0) {
            ss << " [OTHER_OPTIONS ...]";
        }

        size_t prefix_len = USAGE_PREFIX.size();

        std::stringstream wrapped_ss;
        bool first = true;
        for(const auto& line : wrap_width(ss.str(), total_width_ - prefix_len, {" [", " -"})) {
            if(!first) {
                //pass
                wrapped_ss << std::string(prefix_len, ' ');
            }
            wrapped_ss << line;
            first = false;
        }
        wrapped_ss << "\n";

        return wrapped_ss.str();
    }

    std::string DefaultFormatter::format_description() const {
        if (!parser_) throw ArgParseError("parser not initialized in help formatter");

        std::stringstream ss;
        ss << "\n";
        for(auto& line : wrap_width(parser_->description(), total_width_)) {
            ss << line;
        }
        ss << "\n";
        return ss.str();
    }

    std::string DefaultFormatter::format_arguments() const {
        if (!parser_) throw ArgParseError("parser not initialized in help formatter");

        std::stringstream ss;

        for (const auto& group : parser_->argument_groups()) {
            auto args = group.arguments();
            if (args.size() > 0) {
                ss << "\n";
                ss << group.name() << ":" << "\n";
                for (const auto& arg : args) {
                    std::stringstream arg_ss;
                    arg_ss << std::boolalpha;


                    auto long_opt_descr = long_option_str(*arg);
                    auto short_opt_descr = short_option_str(*arg);

                    //name/option
                    arg_ss << INDENT;
                    if (!short_opt_descr.empty()) {
                        arg_ss << short_opt_descr;
                    }
                    if (!long_opt_descr.empty()) {
                        if (!short_opt_descr.empty()) {
                            arg_ss << ", ";
                        }
                        arg_ss << long_opt_descr;
                    }

                    size_t pos = arg_ss.str().size();

                    if (pos + OPTION_HELP_SLACK > option_name_width_) {
                        //If the option name is too long, wrap the help 
                        //around to a new line
                        arg_ss << "\n";
                        pos = 0;
                    }
                    
                    //Argument help
                    auto help_lines = wrap_width(arg->help(), total_width_ - option_name_width_);
                    for (auto& line : help_lines) {
                        //Pad out the help
                        assert(pos <= option_name_width_);
                        arg_ss << std::string(option_name_width_ - pos, ' ');

                        //Print a wrapped line
                        arg_ss << line;
                        pos = 0;
                    }

                    //Default
                    if (!arg->default_value().empty()) {
                        if(!arg->help().empty()) {
                            arg_ss << " ";
                        }
                        arg_ss << "(Default: " << arg->default_value() << ")";
                    }
                    arg_ss << "\n";
                    ss << arg_ss.str();
                }
                if (!group.epilog().empty()) {
                    ss << "\n";

                    auto epilog_lines = wrap_width(group.epilog(), total_width_ - INDENT.size());
                    for (auto& line : epilog_lines) {
                        ss << INDENT << line;
                    }
                    ss << "\n";
                }
            }
        }

        return ss.str();
    }

    std::string DefaultFormatter::format_epilog() const {
        if (!parser_) throw ArgParseError("parser not initialized in help formatter");

        std::stringstream ss;
        ss << "\n";
        for(auto& line : wrap_width(parser_->epilog(), total_width_)) {
            ss << line;
        }
        ss << "\n";
        return ss.str();
    }

    std::string DefaultFormatter::format_version() const {
        if (!parser_) throw ArgParseError("parser not initialized in help formatter");
        return parser_->version() + "\n";
    }

    /*
     * Utilities
     */
    std::string long_option_str(const Argument& argument) {
        auto long_opt = argument.long_option();
        if(argument.nargs() != '0' && !argument.positional()) {
            long_opt += + " " + determine_metavar(argument);
        }
        return long_opt;
    }

    std::string short_option_str(const Argument& argument) {
        auto short_opt = argument.short_option();
        if(!short_opt.empty()) {
            if(argument.nargs() != '0' && !argument.positional()) {
                short_opt += " " + determine_metavar(argument);
            }
            return short_opt;
        } else {
            return "";
        }
    }

    std::string determine_metavar(const Argument& arg) {

        std::string base_metavar = arg.metavar();
        if (!arg.choices().empty()) {
            //We allow choices to override the default metavar
            std::stringstream choices_ss;
            choices_ss << "{";
            bool first = true;
            for(auto choice : arg.choices()) {
                if (!first) {
                    choices_ss << ", ";
                }
                choices_ss << choice;
                first = false;
            }
            choices_ss << "}";
            base_metavar = choices_ss.str();
        }

        std::string metavar;
        if (arg.nargs() == '0' || arg.positional()) {
            //empty
            metavar = "";
        } else if (arg.nargs() == '1') {
            metavar = base_metavar;
        } else if (arg.nargs() == '?') {
            metavar = "[" + base_metavar + "]";
        } else if (arg.nargs() == '+') {
            metavar = base_metavar + " [" + base_metavar + " ...]";
        } else if (arg.nargs() == '*') {
            metavar = "[" + base_metavar + " [" + base_metavar + " ...]]";
        } else {
            assert(false);
        }
        return metavar;
    }

} //namespace
