#include <sstream>
#include "argparse_util.hpp"

namespace argparse {

    template<typename T, typename Converter>
    std::shared_ptr<Argument> make_singlevalue_argument(ArgValue<T>& dest, std::string long_opt, std::string short_opt) {
        auto ptr = std::make_shared<SingleValueArgument<T, Converter>>(dest, long_opt, short_opt);

        //If the conversion object specifies a non-empty set of choices
        //use those by default
        auto default_choices = Converter().default_choices();
        if (!default_choices.empty()) {
            ptr->choices(default_choices);
        }

        return ptr;
    }

    template<typename T, typename Converter>
    std::shared_ptr<Argument> make_multivalue_argument(ArgValue<T>& dest, std::string long_opt, std::string short_opt) {
        auto ptr = std::make_shared<MultiValueArgument<T, Converter>>(dest, long_opt, short_opt);

        //If the conversion object specifies a non-empty set of choices
        //use those by default
        auto default_choices = Converter().default_choices();
        if (!default_choices.empty()) {
            ptr->choices(default_choices);
        }

        return ptr;
    }

    /*
     * ArgumentParser
     */

    template<typename T, typename Converter>
    Argument& ArgumentParser::add_argument(ArgValue<T>& dest, std::string option) {
        return add_argument<T,Converter>(dest, option, std::string());
    }

    template<typename T, typename Converter>
    Argument& ArgumentParser::add_argument(ArgValue<T>& dest, std::string long_opt, std::string short_opt) {
        return argument_groups_[0].add_argument<T,Converter>(dest, long_opt, short_opt);
    }

    template<typename T, typename Converter>
    Argument& ArgumentParser::add_argument(ArgValue<std::vector<T>>& dest, std::string option) {
        return add_argument<T,Converter>(dest, option, std::string());
    }

    template<typename T, typename Converter>
    Argument& ArgumentParser::add_argument(ArgValue<std::vector<T>>& dest, std::string long_opt, std::string short_opt) {
        return argument_groups_[0].add_argument<T,Converter>(dest, long_opt, short_opt);
    }
    /*
     * ArgumentGroup
     */
    template<typename T, typename Converter>
    Argument& ArgumentGroup::add_argument(ArgValue<T>& dest, std::string option) {
        return add_argument<T,Converter>(dest, option, std::string());
    }

    template<typename T, typename Converter>
    Argument& ArgumentGroup::add_argument(ArgValue<T>& dest, std::string long_opt, std::string short_opt) {
        arguments_.push_back(make_singlevalue_argument<T,Converter>(dest, long_opt, short_opt));

        auto& arg = arguments_[arguments_.size() - 1];
        arg->group_name(name()); //Tag the option with the group
        return *arg;
    }

    template<typename T, typename Converter>
    Argument& ArgumentGroup::add_argument(ArgValue<std::vector<T>>& dest, std::string option) {
        return add_argument<T,Converter>(dest, option, std::string());
    }

    template<typename T, typename Converter>
    Argument& ArgumentGroup::add_argument(ArgValue<std::vector<T>>& dest, std::string long_opt, std::string short_opt) {
        arguments_.push_back(make_multivalue_argument<std::vector<T>,Converter>(dest, long_opt, short_opt));

        auto& arg = arguments_[arguments_.size() - 1];
        arg->group_name(name()); //Tag the option with the group
        return *arg;
    }

} //namespace
