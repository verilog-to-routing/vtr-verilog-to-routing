#include <algorithm>
#include <array>
#include <list>
#include <cassert>
#include <string>
#include <set>

#include "argparse.hpp"
#include "argparse_util.hpp"

namespace argparse {


    /*
     * ArgumentParser
     */

    ArgumentParser::ArgumentParser(std::string prog_name, std::string description_str, std::ostream& os)
        : description_(description_str)
        , formatter_(new DefaultFormatter())
        , os_(os)
        {
        prog(prog_name);
        argument_groups_.push_back(ArgumentGroup("arguments"));
    }

    ArgumentParser& ArgumentParser::prog(std::string prog_name, bool basename_only) {
        if (basename_only) {
            prog_ = basename(prog_name);
        } else {
            prog_ = prog_name;
        }
        return *this;
    }

    ArgumentParser& ArgumentParser::version(std::string version_str) {
        version_ = version_str;
        return *this;
    }

    ArgumentParser& ArgumentParser::epilog(std::string epilog_str) {
        epilog_ = epilog_str;
        return *this;
    }

    ArgumentGroup& ArgumentParser::add_argument_group(std::string description_str) {
        argument_groups_.push_back(ArgumentGroup(description_str));
        return argument_groups_[argument_groups_.size() - 1];
    }

    void ArgumentParser::parse_args(int argc, const char* const* argv, int error_exit_code, int help_exit_code, int version_exit_code) {
        try {
            parse_args_throw(argc, argv);
        } catch (const argparse::ArgParseHelp&) {
            //Help requested
            print_help();
            std::exit(help_exit_code);
        } catch (const argparse::ArgParseVersion&) {
            print_version();
            std::exit(version_exit_code);
        } catch (const argparse::ArgParseError& e) {
            //Failed to parse
            std::cout << e.what() << "\n";

            std::cout << "\n";
            print_usage();
            std::exit(error_exit_code);
        }
    }

    void ArgumentParser::parse_args_throw(int argc, const char* const* argv) {
        std::vector<std::string> arg_strs;
        for (int i = 1; i < argc; ++i) {
            arg_strs.push_back(argv[i]);
        }

        parse_args_throw(arg_strs);
    }
    
    void ArgumentParser::parse_args_throw(std::vector<std::string> arg_strs) {
        add_help_option_if_unspecified();

        //Reset all the defaults
        for (const auto& group : argument_groups()) {
            for (const auto& arg : group.arguments()) {
                if (arg->default_set()) {
                    arg->set_dest_to_default();
                }
            }
        }

        //Create a look-up of expected argument strings and positional arguments
        std::map<std::string,std::shared_ptr<Argument>> str_to_option_arg;
        std::list<std::shared_ptr<Argument>> positional_args;
        for (const auto& group : argument_groups()) {
            for (const auto& arg : group.arguments()) {
                if (arg->positional()) {
                    positional_args.push_back(arg);
                } else {
                    for (const auto& opt : {arg->long_option(), arg->short_option()}) {
                        if (opt.empty()) continue;

                        auto ret = str_to_option_arg.insert(std::make_pair(opt, arg));

                        if (!ret.second) {
                            //Option string already specified
                            std::stringstream ss;
                            ss << "Option string '" << opt << "' maps to multiple options";
                            throw ArgParseError(ss.str());
                        }
                    }
                }
            }
        }

        std::set<std::shared_ptr<Argument>> specified_arguments;

        //Process the arguments
        for (size_t i = 0; i < arg_strs.size(); i++) {
            ShortArgInfo short_arg_info = no_space_short_arg(arg_strs[i], str_to_option_arg);

            std::shared_ptr<Argument> arg;

            if (short_arg_info.is_no_space_short_arg) {
                //Short argument with no space between value
                arg = short_arg_info.arg;
            } else { //Full argument
                auto iter = str_to_option_arg.find(arg_strs[i]);
                if (iter != str_to_option_arg.end()) {
                    arg = iter->second;
                }
            }

            if (arg) {
                //Start of an argument

                specified_arguments.insert(arg);

                if (arg->action() == Action::STORE_TRUE) {
                    arg->set_dest_to_true(); 
                } else if (arg->action() == Action::STORE_FALSE) {
                    arg->set_dest_to_false();
                } else if (arg->action() == Action::HELP) {
                    arg->set_dest_to_true(); 
                    throw ArgParseHelp();
                } else if (arg->action() == Action::VERSION) {
                    arg->set_dest_to_true(); 
                    throw ArgParseVersion();
                } else {
                    assert(arg->action() == Action::STORE);


                    size_t max_values_to_read = 0;
                    size_t min_values_to_read = 0;
                    if (arg->nargs() == '1') {
                        max_values_to_read = 1;
                        min_values_to_read = 1;
                    } else if (arg->nargs() == '?') {
                        max_values_to_read = 1;
                        min_values_to_read = 0;
                    } else if (arg->nargs() == '*') {
                        max_values_to_read = std::numeric_limits<size_t>::max();
                        min_values_to_read = 0;
                    } else {
                        assert (arg->nargs() == '+');
                        max_values_to_read = std::numeric_limits<size_t>::max();
                        min_values_to_read = 1;
                    }

                    std::vector<std::string> values;
                    size_t nargs_read = 0;
                    if (short_arg_info.is_no_space_short_arg) {
                        //It is a short argument, we already have the first value
                        if (!short_arg_info.value.empty()) {
                            values.push_back(short_arg_info.value);
                            ++nargs_read;
                        }
                    }
                    for (; nargs_read < max_values_to_read; ++nargs_read) {
                        size_t next_idx = i + 1 + nargs_read;
                        if (next_idx >= arg_strs.size()) {
                            break;
                        }
                        std::string str = arg_strs[next_idx];


                        if (is_argument(str, str_to_option_arg)) break;

                        if (!arg->is_valid_value(str)) break;

                        values.push_back(str);
                    }

                    if (nargs_read < min_values_to_read) {

                        if (arg->nargs() == '1') {
                            std::stringstream msg;
                            msg << "Missing expected argument for " << arg_strs[i] << "";
                            throw ArgParseError(msg.str());

                        } else {
                            std::stringstream msg;
                            msg << "Expected at least " << min_values_to_read << " value";
                            if (min_values_to_read > 1) {
                                msg << "s";
                            }
                            msg << " for argument '" << arg_strs[i] << "'";
                            msg << " (found " << values.size() << ")";
                            throw ArgParseError(msg.str());
                        }
                    }
                    assert (nargs_read <= max_values_to_read);

                    for (auto val : values) {
                        if (!is_valid_choice(val, arg->choices())) {
                            std::stringstream msg;
                            msg << "Unexpected option value '" << values[0] << "' (expected one of: " << join(arg->choices(), ", ");
                            msg << ") for " << arg->name();
                            throw ArgParseError(msg.str());
                        }
                    }

                    //Set the option values appropriately
                    if (arg->nargs() == '1') {
                        assert(nargs_read == 1);
                        assert(values.size() == 1);


                        try {
                            arg->set_dest_to_value(values[0]); 
                        } catch (const ArgParseConversionError& e) {
                            std::stringstream msg;
                            msg << e.what() << " for " << arg->long_option();
                            auto short_opt = arg->short_option();
                            if (!short_opt.empty()) {
                                msg << "/" << short_opt;
                            }
                            throw ArgParseConversionError(msg.str());
                        }
                    } else if (arg->nargs() == '+' || arg->nargs() == '*') {
                        if (arg->nargs() == '+') {
                            assert(nargs_read >= 1);
                            assert(values.size() >= 1);
                        }

                        for (auto value : values) {
                            try {
                                arg->add_value_to_dest(value); 
                            } catch (const ArgParseConversionError& e) {
                                std::stringstream msg;
                                msg << e.what() << " for " << arg->long_option();
                                auto short_opt = arg->short_option();
                                if (!short_opt.empty()) {
                                    msg << "/" << short_opt;
                                }
                                throw ArgParseConversionError(msg.str());
                            }
                        }
                    } else {
                        std::stringstream msg;
                        msg << "Unsupport nargs value '" << arg->nargs() << "'";
                        throw ArgParseError(msg.str());
                    }

                    if (!short_arg_info.is_no_space_short_arg) {
                        i += nargs_read; //Skip over the values (don't need to for short args)
                    }
                }

            } else {
                if (positional_args.empty()) {
                    //Unrecognized
                    std::stringstream ss;
                    ss << "Unexpected command-line argument '" << arg_strs[i] << "'";
                    throw ArgParseError(ss.str());
                } else {
                    //Positional argument
                    auto pos_arg = positional_args.front();
                    positional_args.pop_front();

                    try {
                        pos_arg->set_dest_to_value(arg_strs[i]); 
                    } catch (const ArgParseConversionError& e) {
                        std::stringstream msg;
                        msg << e.what() << " for positional argument " << pos_arg->long_option();
                        throw ArgParseConversionError(msg.str());
                    }

                    auto value = arg_strs[i];
                    specified_arguments.insert(pos_arg);
                }
            }
        }

        //Missing positionals?
        for(const auto& remaining_positional : positional_args) {
            std::stringstream ss;
            ss << "Missing required positional argument: " << remaining_positional->long_option();
            throw ArgParseError(ss.str());
        }

        //Missing required?
        for (const auto& group : argument_groups()) {
            for (const auto& arg : group.arguments()) {
                if (arg->required()) {
                    //potentially slow...
                    if (!specified_arguments.count(arg)) {
                        std::stringstream msg;
                        msg << "Missing required argument: " << arg->long_option();
                        auto short_opt = arg->short_option();
                        if (!short_opt.empty()) {
                            msg << "/" << short_opt;
                        }
                        throw ArgParseError(msg.str());
                    }
                }
            }
        }
    }

    void ArgumentParser::reset_destinations() {
        for (const auto& group : argument_groups()) {
            for (const auto& arg : group.arguments()) {
                arg->reset_dest();
            }
        }
    }

    void ArgumentParser::print_usage() {
        formatter_->set_parser(this);
        os_ << formatter_->format_usage();
    }

    void ArgumentParser::print_help() {
        formatter_->set_parser(this);
        os_ << formatter_->format_usage();
        os_ << formatter_->format_description();
        os_ << formatter_->format_arguments();
        os_ << formatter_->format_epilog();
    }

    void ArgumentParser::print_version() {
        formatter_->set_parser(this);
        os_ << formatter_->format_version();
    }

    std::string ArgumentParser::prog() const { return prog_; }
    std::string ArgumentParser::version() const { return version_; }
    std::string ArgumentParser::description() const { return description_; }
    std::string ArgumentParser::epilog() const { return epilog_; }
    std::vector<ArgumentGroup> ArgumentParser::argument_groups() const { return argument_groups_; }

    void ArgumentParser::add_help_option_if_unspecified() {
        //Has a help already been specified
        bool found_help = false;
        for(auto& grp : argument_groups_) {
            for(auto& arg : grp.arguments()) {
                if(arg->action() == Action::HELP) {
                    found_help = true;
                    break;
                }
            }
        }

        if (!found_help) {

            auto& grp = argument_groups_[0];

            grp.add_argument(show_help_dummy_, "--help", "-h")
                .help("Shows this help message")
                .action(Action::HELP);
        }
    }

    ArgumentParser::ShortArgInfo ArgumentParser::no_space_short_arg(std::string str, const std::map<std::string, std::shared_ptr<Argument>>& str_to_option_arg) const {

        ShortArgInfo short_arg_info;
        for(auto kv : str_to_option_arg) {
            if (kv.first.size() == 2) {
                //Is a short arg
                
                //String starts with short arg
                bool match = true;
                for (size_t i = 0; i < kv.first.size(); ++i) {
                    if (kv.first[i] != str[i]) {
                        match = false;
                        break;
                    }
                }

                bool no_space_between_short_arg_and_value = str.size() > kv.first.size();

                if (match && no_space_between_short_arg_and_value) {
                    //Only handles cases where there is no space between short arg and value
                    short_arg_info.is_no_space_short_arg = true;
                    short_arg_info.arg = kv.second;
                    short_arg_info.value = std::string(str.begin() + kv.first.size(), str.end());

                    return short_arg_info;
                }
            }
        }

        assert(!short_arg_info.is_no_space_short_arg);
        return short_arg_info;
    }

    /*
     * ArgumentGroup
     */
    ArgumentGroup::ArgumentGroup(std::string name_str)
        : name_(name_str)
        {}

    ArgumentGroup& ArgumentGroup::epilog(std::string str) {
        epilog_ = str;
        return *this;
    }
    std::string ArgumentGroup::name() const { return name_; }
    std::string ArgumentGroup::epilog() const { return epilog_; }
    const std::vector<std::shared_ptr<Argument>>& ArgumentGroup::arguments() const { return arguments_; }

    /*
     * Argument
     */
    Argument::Argument(std::string long_opt, std::string short_opt)
        : long_opt_(long_opt)
        , short_opt_(short_opt) {

        if (long_opt_.size() < 1) {
            throw ArgParseError("Argument must be at least one character long");
        }

        auto dashes_name = split_leading_dashes(long_opt_);

        if (dashes_name[0].size() == 1 && !short_opt_.empty()) {
            throw ArgParseError("Long option must be specified before short option");
        } else if (dashes_name[0].size() > 2) {
            throw ArgParseError("More than two dashes in argument name");
        }

        //Set defaults
        metavar_ = toupper(dashes_name[1]);
    }

    Argument& Argument::help(std::string help_str) {
        help_ = help_str;
        return *this;
    }

    Argument& Argument::nargs(char nargs_type) {
        //TODO: nargs > 1 support: '?', '*', '+'
        auto valid_nargs = {'0', '1', '+', '*'};

        auto iter = std::find(valid_nargs.begin(), valid_nargs.end(), nargs_type);
        if (iter == valid_nargs.end()) {
            throw ArgParseError("Invalid argument to nargs (must be one of: " + join(valid_nargs, ", ") + ")");
        }

        //Ensure nargs is consistent with the action
        if (action() == Action::STORE_FALSE && nargs_type != '0') {
            throw ArgParseError("STORE_FALSE action requires nargs to be '0'");
        } else if (action() == Action::STORE_TRUE && nargs_type != '0') {
            throw ArgParseError("STORE_TRUE action requires nargs to be '0'");
        } else if (action() == Action::HELP && nargs_type != '0') {
            throw ArgParseError("HELP action requires nargs to be '0'");
        } else if (action() == Action::STORE && (nargs_type != '1' && nargs_type != '+' && nargs_type != '*')) {
            throw ArgParseError("STORE action requires nargs to be '1', '+' or '*'");
        }

        nargs_ = nargs_type;

        valid_action();
        return *this;
    }

    Argument& Argument::metavar(std::string metavar_str) {
        metavar_ = metavar_str;
        return *this;
    }

    Argument& Argument::choices(std::vector<std::string> choice_values) {
        choices_ = choice_values;
        return *this;
    }

    Argument& Argument::action(Action action_type) {
        action_ = action_type;

        if (   action_ == Action::STORE_FALSE 
            || action_ == Action::STORE_TRUE 
            || action_ == Action::HELP 
            || action_ == Action::VERSION) {
            this->nargs('0');
        } else if (action_ == Action::STORE) {
            this->nargs('1');
        } else {
            throw ArgParseError("Unrecognized argparse action");
        }

        return *this;
    }

    Argument& Argument::required(bool is_required) {
        required_ = is_required;
        return *this;
    }

    Argument& Argument::default_value(const std::string& value) {
        if (nargs() != '0' && nargs() != '1' && nargs() != '?') {
            std::stringstream msg;
            msg << "Scalar default value not allowed for nargs='" << nargs() << "'";
            throw ArgParseError(msg.str());
        }
        default_value_.clear();
        default_value_.push_back(value);
        default_set_ = true;
        return *this;
    }

    Argument& Argument::default_value(const std::vector<std::string>& values) {
        if (nargs() != '+' && nargs() != '*') {
            std::stringstream msg;
            msg << "Multiple default value not allowed for nargs='" << nargs() << "'";
            throw ArgParseError(msg.str());
        }
        default_value_ = values;
        default_set_ = true;
        return *this;
    }

    Argument& Argument::default_value(const std::initializer_list<std::string>& values) {
        //Convert to vector and process as usual
        return default_value(std::vector<std::string>(values.begin(), values.end()));
    }

    Argument& Argument::group_name(std::string grp) {
        group_name_ = grp;
        return *this;
    }

    Argument& Argument::show_in(ShowIn show) {
        show_in_ = show;
        return *this;
    }

    std::string Argument::name() const { 
        std::string name_str = long_option();
        if (!short_option().empty()) {
            name_str += "/" + short_option();
        }
        return name_str;
    }

    std::string Argument::long_option() const { return long_opt_; }
    std::string Argument::short_option() const { return short_opt_; }
    std::string Argument::help() const { return help_; }
    char Argument::nargs() const { return nargs_; }
    std::string Argument::metavar() const { return metavar_; }
    std::vector<std::string> Argument::choices() const { return choices_; }
    Action Argument::action() const { return action_; }
    std::string Argument::default_value() const { 
        if (default_value_.size() > 1) {
            std::stringstream msg;
            msg << "{" << join(default_value_, ", ") << "}";
            return msg.str();
        } else if (default_value_.size() == 1) {
            return default_value_[0]; 
        } else {
            return "";
        }
    }
    std::string Argument::group_name() const { return group_name_; }
    ShowIn Argument::show_in() const { return show_in_; }
    bool Argument::default_set() const { return default_set_; }

    bool Argument::required() const {
        if(positional()) {
            //Positional arguments are always required
            return true;
        }
        return required_;
    }
    bool Argument::positional() const {
        assert(long_option().size() > 1);
        return long_option()[0] != '-';
    }
} //namespace
