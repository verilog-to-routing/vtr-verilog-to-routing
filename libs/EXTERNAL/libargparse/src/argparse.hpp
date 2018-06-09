#ifndef ARGPARSE_H
#define ARGPARSE_H
#include <iosfwd>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <memory>
#include <map>

#include "argparse_formatter.hpp"
#include "argparse_default_converter.hpp"
#include "argparse_error.hpp"
#include "argparse_value.hpp"

namespace argparse {

    class Argument;
    class ArgumentGroup;

    enum class Action {
        STORE,
        STORE_TRUE,
        STORE_FALSE,
        HELP,
        VERSION
    };

    enum class ShowIn {
        USAGE_AND_HELP,
        HELP_ONLY
    };

    class ArgumentParser {
        public:
            //Initializes an argument parser
            ArgumentParser(std::string prog_name, std::string description_str=std::string(), std::ostream& os=std::cout);

            //Overrides the program name
            ArgumentParser& prog(std::string prog, bool basename_only=true);

            //Sets the program version
            ArgumentParser& version(std::string version);

            //Specifies the epilog text at the bottom of the help description
            ArgumentParser& epilog(std::string prog);

            //Adds an argument or option with a single name (single value)
            template<typename T, typename Converter=DefaultConverter<T>>
            Argument& add_argument(ArgValue<T>& dest, std::string option);

            //Adds an option with a long and short option name (single value)
            template<typename T, typename Converter=DefaultConverter<T>>
            Argument& add_argument(ArgValue<T>& dest, std::string long_opt, std::string short_opt);

            //Adds an argument or option with a single name (multi value)
            template<typename T, typename Converter=DefaultConverter<T>>
            Argument& add_argument(ArgValue<std::vector<T>>& dest, std::string option);

            //Adds an option with a long and short option name (multi value)
            template<typename T, typename Converter=DefaultConverter<T>>
            Argument& add_argument(ArgValue<std::vector<T>>& dest, std::string long_opt, std::string short_opt);

            //Adds a group to collect related arguments
            ArgumentGroup& add_argument_group(std::string description_str);

            //Like parse_arg_throw(), but catches exceptions and exits the program
            void parse_args(int argc, const char* const* argv, int error_exit_code=1, int help_exit_code=0, int version_exit_code=0);

            //Parses the specified command-line arguments and sets the appropriat argument values
            // Returns a vector of Arguments which were specified.
            //If an error occurs throws ArgParseError
            //If an help is requested occurs throws ArgParseHelp
            void parse_args_throw(int argc, const char* const* argv);
            void parse_args_throw(std::vector<std::string> args);

            //Reset the target values to their initial state
            void reset_destinations();

            //Prints the basic usage
            void print_usage();

            //Prints the usage and full help description for each option
            void print_help();

            //Prints the version information
            void print_version();
        public:
            //Returns the program name
            std::string prog() const;

            std::string version() const;

            //Returns the program description (after usage, but before option descriptions)
            std::string description() const;

            //Returns the epilog (end of help)
            std::string epilog() const;

            //Returns all the argument groups in this parser
            std::vector<ArgumentGroup> argument_groups() const;

        private:
            void add_help_option_if_unspecified();

            struct ShortArgInfo {
                bool is_no_space_short_arg = false;
                std::shared_ptr<argparse::Argument> arg;
                std::string value;
            };
            ShortArgInfo no_space_short_arg(std::string str, const std::map<std::string, std::shared_ptr<Argument>>& str_to_option_arg) const;
        private:
            std::string prog_;
            std::string description_;
            std::string epilog_;
            std::string version_;
            std::vector<ArgumentGroup> argument_groups_;

            std::unique_ptr<Formatter> formatter_;
            std::ostream& os_;
            ArgValue<bool> show_help_dummy_; //Dummy variable used as destination for automatically generated help option
    };

    class ArgumentGroup {
        public:

            //Adds an argument or option with a single name (single value)
            template<typename T, typename Converter=DefaultConverter<T>>
            Argument& add_argument(ArgValue<T>& dest, std::string option);

            //Adds an option with a long and short option name (single value)
            template<typename T, typename Converter=DefaultConverter<T>>
            Argument& add_argument(ArgValue<T>& dest, std::string long_opt, std::string short_opt);

            //Adds an argument or option with a multi name (multi value)
            template<typename T, typename Converter=DefaultConverter<T>>
            Argument& add_argument(ArgValue<std::vector<T>>& dest, std::string option);

            //Adds an option with a long and short option name (multi value)
            template<typename T, typename Converter=DefaultConverter<T>>
            Argument& add_argument(ArgValue<std::vector<T>>& dest, std::string long_opt, std::string short_opt);

            //Adds an epilog to the group
            ArgumentGroup& epilog(std::string str);

        public:
            //Returns the name of the group
            std::string name() const;

            //Returns the epilog
            std::string epilog() const;

            //Returns the arguments within the group
            const std::vector<std::shared_ptr<Argument>>& arguments() const;
        public:
            ArgumentGroup(const ArgumentGroup&) = default;
            ArgumentGroup(ArgumentGroup&&) = default;
            ArgumentGroup& operator=(const ArgumentGroup&) = delete;
            ArgumentGroup& operator=(const ArgumentGroup&&) = delete;
        private:
            friend class ArgumentParser;
            ArgumentGroup(std::string name_str=std::string());
        private:
            std::string name_;
            std::string epilog_;
            std::vector<std::shared_ptr<Argument>> arguments_;
    };

    class Argument {
        public:
            Argument(std::string long_opt, std::string short_opt);
        public: //Configuration Mutators
            //Sets the hlep text
            Argument& help(std::string help_str);

            //Sets the defuault value
            Argument& default_value(const std::string& default_val);
            Argument& default_value(const std::vector<std::string>& default_val);
            Argument& default_value(const std::initializer_list<std::string>& default_val);

            //Sets the action
            Argument& action(Action action);

            //Sets whether this argument is required
            Argument& required(bool is_required);

            //Sets the associated metavar (if not specified, inferred from argument name, or choices)
            Argument& metavar(std::string metavar_sr);

            //Sets the expected number of arguments
            Argument& nargs(char nargs_type);

            //Sets the valid choices for this option's value
            Argument& choices(std::vector<std::string> choice_values);

            //Sets the group name this argument is associated with
            Argument& group_name(std::string grp);

            //Sets where this option appears in the help
            Argument& show_in(ShowIn show);

        public: //Option setting mutators
            //Sets the target value to the specified default
            virtual void set_dest_to_default() = 0;

            //Sets the target value to the specified value
            virtual void set_dest_to_value(std::string value) = 0;

            //Adds the specified value to the taget values
            virtual void add_value_to_dest(std::string value) = 0;

            //Set the target value to true
            virtual void set_dest_to_true() = 0;

            //Set the target value to false
            virtual void set_dest_to_false() = 0;

            virtual void reset_dest() = 0;
        public: //Accessors

            //Returns a discriptive name build from the long/short option
            std::string name() const;

            //Returns the long option name (or positional name) for this argument.
            //Note that this may be a single-letter option if only a short option name was specified
            std::string long_option() const;

            //Returns the short option name for this argument, note that this returns
            //the empty string if no short option is specified, or if only the short option
            //is specified.
            std::string short_option() const;

            //Returns the help description for this option
            std::string help() const;

            //Returns the number of arguments this option expects
            char nargs() const;

            //Returns the specified metavar for this option
            std::string metavar() const;

            //Returns the list of valid choices for this option
            std::vector<std::string> choices() const;

            //Returns the action associated with this option
            Action action() const;

            //Returns whether this option is required
            bool required() const;

            //Returns the specified default value
            std::string default_value() const;

            //Returns the group name associated with this argument
            std::string group_name() const;

            //Indicates where this option should appear in the help
            ShowIn show_in() const;

            //Returns true if this is a positional argument
            bool positional() const;

            //Returns true if the default_value() was set
            bool default_set() const;

            //Returns true if the proposed value is legal
            virtual bool is_valid_value(std::string value) = 0;
        public: //Lifetime
            virtual ~Argument() {}
            Argument(const Argument&) = default;
            Argument(Argument&&) = default;
            Argument& operator=(const Argument&) = delete;
            Argument& operator=(const Argument&&) = delete;
        protected:
            virtual bool valid_action() = 0;
            std::vector<std::string> default_value_;
        private: //Data
            std::string long_opt_;
            std::string short_opt_;

            std::string help_;
            std::string metavar_;
            char nargs_ = '1';
            std::vector<std::string> choices_;
            Action action_ = Action::STORE;
            bool required_ = false;


            std::string group_name_;
            ShowIn show_in_ = ShowIn::USAGE_AND_HELP;
            bool default_set_ = false;
    };

    template<typename T, typename Converter>
    class SingleValueArgument : public Argument {
        public: //Constructors
            SingleValueArgument(ArgValue<T>& dest, std::string long_opt, std::string short_opt)
                : Argument(long_opt, short_opt)
                , dest_(dest)
                {}
        public: //Mutators
            void set_dest_to_default() override {
                dest_.set(Converter().from_str(default_value()), Provenance::DEFAULT);
                dest_.set_argument_name(name());
                dest_.set_argument_group(group_name());
            }

            void set_dest_to_value(std::string value) override {
                if (dest_.provenance() == Provenance::SPECIFIED
                    && dest_.argument_name() == name()) {
                    throw ArgParseError("Argument " + name() + " specified multiple times");
                }

                dest_.set(Converter().from_str(value), Provenance::SPECIFIED);
                dest_.set_argument_name(name());
                dest_.set_argument_group(group_name());
            }

            void add_value_to_dest(std::string /*value*/) override {
                throw ArgParseError("Single value option can not have multiple values set");
            }

            void set_dest_to_true() override {
                throw ArgParseError("Non-boolean destination can not be set true");
            }
            void set_dest_to_false() override {
                throw ArgParseError("Non-boolean destination can not be set false");
            }

            bool valid_action() override {
                //Sanity check that we aren't processing a boolean action with a non-boolean destination
                if (action() == Action::STORE_TRUE) {
                    std::stringstream msg;
                    msg << "Non-boolean destination can not have STORE_TRUE action (" << long_option() << ")";
                    throw ArgParseError(msg.str());
                } else if (action() == Action::STORE_FALSE) {
                    std::stringstream msg;
                    msg << "Non-boolean destination can not have STORE_FALSE action (" << long_option() << ")";
                    throw ArgParseError(msg.str());
                } else if (action() != Action::STORE) {
                    throw ArgParseError("Unexpected action (expected STORE)");
                }
                return true;
            }

            void reset_dest() override {
                dest_ = ArgValue<T>();
            }

            bool is_valid_value(std::string value) override {
                auto converted_value = Converter().from_str(value);

                if (!converted_value) {
                    return false;
                }
                return is_valid_choice(value, choices());
            }

        private: //Data
            ArgValue<T>& dest_;
    };

    //bool specialization for STORE_TRUE/STORE_FALSE
    template<typename Converter>
    class SingleValueArgument<bool,Converter> : public Argument {
        public: //Constructors
            SingleValueArgument(ArgValue<bool>& dest, std::string long_opt, std::string short_opt)
                : Argument(long_opt, short_opt)
                , dest_(dest)
                {}
        public: //Mutators
            void set_dest_to_default() override {
                dest_.set(Converter().from_str(default_value()), Provenance::DEFAULT);
                dest_.set_argument_name(name());
                dest_.set_argument_group(group_name());
            }

            void add_value_to_dest(std::string /*value*/) override {
                throw ArgParseError("Single value option can not have multiple values set");
            }

            void set_dest_to_value(std::string value) override {
                if (dest_.provenance() == Provenance::SPECIFIED
                    && dest_.argument_name() == name()) {
                    throw ArgParseError("Argument " + name() + " specified multiple times");
                }

                dest_.set(Converter().from_str(value), Provenance::SPECIFIED);
                dest_.set_argument_name(name());
                dest_.set_argument_group(group_name());
            }

            void set_dest_to_true() override {
                ConvertedValue<bool> val;
                val.set_value(true);

                dest_.set(val, Provenance::SPECIFIED);
                dest_.set_argument_name(name());
                dest_.set_argument_group(group_name());
            }

            void set_dest_to_false() override {
                ConvertedValue<bool> val;
                val.set_value(false);

                dest_.set(val, Provenance::SPECIFIED);
                dest_.set_argument_name(name());
                dest_.set_argument_group(group_name());
            }

            bool valid_action() override { 
                //Any supported action is valid on a boolean destination
                return true; 
            }

            void reset_dest() override {
                dest_ = ArgValue<bool>();
            }

            bool is_valid_value(std::string value) override {
                auto converted_value = Converter().from_str(value);

                if (!converted_value) {
                    return false;
                }
                return is_valid_choice(value, choices());
            }
        private: //Data
            ArgValue<bool>& dest_;
    };

    template<typename T, typename Converter>
    class MultiValueArgument : public Argument {
        public: //Constructors
            MultiValueArgument(ArgValue<T>& dest, std::string long_opt, std::string short_opt)
                : Argument(long_opt, short_opt)
                , dest_(dest)
                {}

        public: //Mutators
            void set_dest_to_default() override {
                auto& target = dest_.mutable_value(Provenance::DEFAULT);
                for (auto default_str : default_value_) {
                    auto val = Converter().from_str(default_str);
                    target.insert(std::end(target), val.value());
                }

                dest_.set_argument_name(name());
                dest_.set_argument_group(group_name());
            }

            void set_dest_to_value(std::string /*value*/) override {
                throw ArgParseError("Multi-value option can not be set to a single value");
            }

            void add_value_to_dest(std::string value) override {
                if (dest_.provenance() == Provenance::SPECIFIED
                    && dest_.argument_name() != name()) {
                    throw ArgParseError("Argument destination already set by " + dest_.argument_name() + " (trying to set from " + name() + ")");
                }

                auto previous_provenance = dest_.provenance();

                auto& target = dest_.mutable_value(Provenance::SPECIFIED);

                if (previous_provenance == Provenance::DEFAULT) {
                    target.clear();
                }

                //Insert is more general than push_back
                auto converted_value = Converter().from_str(value);
                if (!converted_value) {
                    throw ArgParseConversionError(converted_value.error());
                }
                target.insert(std::end(target), converted_value.value());

                dest_.set_argument_name(name());
                dest_.set_argument_group(group_name());
            }

            void set_dest_to_true() override {
                throw ArgParseError("Non-boolean destination can not be set true");
            }
            void set_dest_to_false() override {
                throw ArgParseError("Non-boolean destination can not be set false");
            }

            bool valid_action() override {
                //Sanity check that we aren't processing a boolean action with a non-boolean destination
                if (action() != Action::STORE) {
                    throw ArgParseError("Unexpected action (expected STORE)");
                }
                return true;
            }

            void reset_dest() override {
                dest_ = ArgValue<T>();
            }

            bool is_valid_value(std::string value) override {
                auto converted_value = Converter().from_str(value);

                if (!converted_value) {
                    return false;
                }
                return is_valid_choice(value, choices());
            }
        private: //Data
            ArgValue<T>& dest_;
    };


} //namespace

#include "argparse.tpp"

#endif
