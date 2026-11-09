/**
 * @file read_options_file.cpp
 * @brief Converts a YAML options file (--options_file) into command-line argument strings.
 */

#include "read_options_file.h"

#include <map>
#include <memory>
#include <sstream>

#include <yaml-cpp/yaml.h>

using argparse::ArgParseError;
using argparse::Argument;

namespace {

/**
 * @brief One key/value entry of the options file.
 */
struct t_options_file_entry {
    YAML::Node key;   ///< The option name, as written in the file.
    YAML::Node value; ///< The option value, as written in the file.
};

/**
 * @brief The arguments of a parser, for looking up the keys of an options file.
 */
struct t_parser_args {
    std::vector<std::shared_ptr<const Argument>> positionals;   ///< Positional arguments, in the order the parser expects them.
    std::map<std::string, std::shared_ptr<const Argument>> map; ///< All arguments, by positional name (e.g. "circuit") or option string (e.g. "--seed", "-j").
};

} // namespace

/**
 * @brief Builds the message for an error at the given position in the options file.
 */
static std::string yaml_error_message(const YAML::Mark& mark, const std::string& msg);

/**
 * @brief Collects the arguments of the parser.
 */
static t_parser_args get_parser_args(const argparse::ArgumentParser& parser);

/**
 * @brief Returns the argument named by a key of the options file, or nullptr if there is none.
 *
 * The leading "--" of an option is optional (e.g. "seed" and "--seed" both give --seed).
 */
static std::shared_ptr<const Argument> find_argument(const t_parser_args& parser_args, const std::string& key);

/**
 * @brief Returns the text of a scalar value, exactly as written in the file.
 *
 * @param entry The entry the value belongs to (used for error messages).
 * @param value The value; either the entry's value, or an item of it if it is a sequence.
 */
static std::string get_scalar_value(const t_options_file_entry& entry, const YAML::Node& value);

/**
 * @brief Appends the command-line arguments for one option of the options file.
 *
 * @param entry  The option's entry in the file.
 * @param option The option the entry's key refers to.
 * @param args   The arguments to append to.
 */
static void append_option_args(const t_options_file_entry& entry,
                               const Argument& option,
                               std::vector<std::string>& args);

/**
 * @brief Returns the values of the positional arguments, in the order the parser expects them.
 *
 * Returns an empty vector if the file gives no positional arguments, and throws if it gives only
 * some of them; a partial set would be matched to the wrong positions.
 *
 * @param positional_args    The parser's positional arguments.
 * @param positional_entries The entries of the file which give positional arguments, keyed by name.
 */
static std::vector<std::string> get_positional_values(const std::vector<std::shared_ptr<const Argument>>& positional_args,
                                                      const std::map<std::string, t_options_file_entry>& positional_entries);

std::vector<std::string> expand_yaml_options_file(const std::string& file_path,
                                                  const argparse::ArgumentParser& parser) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(file_path);
    } catch (const YAML::Exception& e) {
        throw ArgParseError(yaml_error_message(e.mark, e.msg));
    }

    // An empty file has no options.
    if (root.IsNull()) {
        return {};
    }
    if (!root.IsMap()) {
        throw ArgParseError(yaml_error_message(root.Mark(), "expected a map of option names to values"));
    }

    t_parser_args parser_args = get_parser_args(parser);

    // Positional arguments must come before the options, so collect them separately.
    std::map<std::string, t_options_file_entry> positional_entries;
    std::vector<std::string> option_args;
    for (const auto& key_value : root) {
        t_options_file_entry entry{key_value.first, key_value.second};
        if (!entry.key.IsScalar()) {
            throw ArgParseError(yaml_error_message(entry.key.Mark(), "expected an option name"));
        }
        std::string key = entry.key.Scalar();

        std::shared_ptr<const Argument> arg = find_argument(parser_args, key);
        if (!arg) {
            throw ArgParseError(yaml_error_message(entry.key.Mark(), "unknown option '" + key + "'"));
        }

        if (arg->positional()) {
            positional_entries[key] = entry;
        } else {
            append_option_args(entry, *arg, option_args);
        }
    }

    std::vector<std::string> args = get_positional_values(parser_args.positionals, positional_entries);
    args.insert(args.end(), option_args.begin(), option_args.end());
    return args;
}

static std::string yaml_error_message(const YAML::Mark& mark, const std::string& msg) {
    std::stringstream ss;
    if (!mark.is_null()) {
        // Marks are zero-based.
        ss << "line " << mark.line + 1 << ": ";
    }
    ss << msg;
    return ss.str();
}

static t_parser_args get_parser_args(const argparse::ArgumentParser& parser) {
    t_parser_args parser_args;
    for (const argparse::ArgumentGroup& group : parser.argument_groups()) {
        for (const std::shared_ptr<Argument>& arg : group.arguments()) {
            // A positional argument's long option is its name.
            parser_args.map[arg->long_option()] = arg;
            if (arg->positional()) {
                parser_args.positionals.push_back(arg);
            } else if (!arg->short_option().empty()) {
                parser_args.map[arg->short_option()] = arg;
            }
        }
    }
    return parser_args;
}

static std::shared_ptr<const Argument> find_argument(const t_parser_args& parser_args, const std::string& key) {
    auto it = parser_args.map.find(key);
    if (it == parser_args.map.end() && (key.empty() || key[0] != '-')) {
        it = parser_args.map.find("--" + key);
    }
    if (it == parser_args.map.end()) {
        return nullptr;
    }
    return it->second;
}

static std::string get_scalar_value(const t_options_file_entry& entry, const YAML::Node& value) {
    // A missing value has no useful position of its own, so report the key's line.
    if (value.IsNull()) {
        throw ArgParseError(yaml_error_message(entry.key.Mark(), "expected a value for '" + entry.key.Scalar() + "'"));
    }
    if (!value.IsScalar()) {
        throw ArgParseError(yaml_error_message(value.Mark(), "expected a single value for '" + entry.key.Scalar() + "'"));
    }
    return value.Scalar();
}

static void append_option_args(const t_options_file_entry& entry,
                               const Argument& option,
                               std::vector<std::string>& args) {
    // Flags (e.g. --pack): YAML true adds the flag and false leaves it out.
    if (option.nargs() == '0') {
        bool is_set = false;
        if (!YAML::convert<bool>::decode(entry.value, is_set)) {
            throw ArgParseError(yaml_error_message(entry.key.Mark(), "expected true or false for flag '" + entry.key.Scalar() + "'"));
        }
        if (is_set) {
            args.push_back(option.long_option());
        }
        return;
    }

    args.push_back(option.long_option());

    bool is_multi_value = (option.nargs() == '+' || option.nargs() == '*');
    if (is_multi_value && entry.value.IsMap()) {
        throw ArgParseError(yaml_error_message(entry.value.Mark(), "expected a value or a list of values for '" + entry.key.Scalar() + "'"));
    }
    if (is_multi_value && entry.value.IsSequence()) {
        for (const YAML::Node& item : entry.value) {
            args.push_back(get_scalar_value(entry, item));
        }
        return;
    }

    // The value is passed on as the raw text from the file rather than as a YAML-typed value, so
    // that it is converted and checked by the option's own converter, exactly as on the
    // command-line (e.g. "on" stays "on" rather than becoming the YAML boolean true).
    args.push_back(get_scalar_value(entry, entry.value));
}

static std::vector<std::string> get_positional_values(const std::vector<std::shared_ptr<const Argument>>& positional_args,
                                                      const std::map<std::string, t_options_file_entry>& positional_entries) {
    std::vector<std::string> values;
    if (positional_entries.empty()) {
        return values;
    }

    for (const std::shared_ptr<const Argument>& positional_arg : positional_args) {
        std::string name = positional_arg->long_option();
        auto it = positional_entries.find(name);
        if (it == positional_entries.end()) {
            // Report the line of a positional argument which is given.
            const YAML::Node& given_key = positional_entries.begin()->second.key;
            throw ArgParseError(yaml_error_message(given_key.Mark(), "missing '" + name + "' (if any positional argument is given in the file, all must be given)"));
        }
        values.push_back(get_scalar_value(it->second, it->second.value));
    }
    return values;
}
