/**
 * @file
 * @author  Alex Singer
 * @date    June 2025
 * @brief   Implementation of utility functions used for parsing AP arguments.
 */
#include "ap_argparse_utils.h"
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vpr_error.h"

/**
 * @brief Helper method to convert a string into a float with error checking.
 */
static float str_to_float_or_error(const std::string& str);

/**
 * @brief Parse the given key, value string argument. The string is expected to
 *        be of the form:
 *              "<key_regex>:<val1>,<val2>,<val3>"
 *
 * This method returns a tuple containing the regex string and a vector of the
 * values. The vector will be of the length expected_num_vals_per_key.
 */
static std::tuple<std::string, std::vector<float>>
parse_key_val_arg(const std::string& arg, unsigned expected_num_vals_per_key);

std::unordered_map<std::string, std::vector<float>>
key_to_float_argument_parser(const std::vector<std::string>& arg_vals,
                             const std::vector<std::string>& valid_keys,
                             unsigned expected_num_vals_per_key) {

    // Create the key to float map which will be returned from this method.
    std::unordered_map<std::string, std::vector<float>> key_to_float_map;

    // Go through each of the arguments to parse.
    for (const std::string& arg_val : arg_vals) {
        // Parse this argument.
        // Key is the regex string, vals is the vector of values.
        auto [key, vals] = parse_key_val_arg(arg_val, expected_num_vals_per_key);

        // Create a regex object to be used to match for valid keys.
        std::regex key_regex(key);

        // Go through each valid key and find which ones match the regex.
        bool found_match = false;
        for (const std::string& valid_key : valid_keys) {
            bool is_match = std::regex_match(valid_key, key_regex);
            if (!is_match)
                continue;

            // If this key matches the regex, set the map to the given values.
            key_to_float_map[valid_key] = vals;
            found_match = true;
        }

        // If no match is found for this key regex, raise a warning to the user.
        // They may have made a mistake and may want to be warned about it.
        if (!found_match) {
            VTR_LOG_WARN("Unable to find a valid key that matches regex pattern: %s\n",
                         key.c_str());
        }
    }

    // Return the map.
    return key_to_float_map;
}

static std::tuple<std::string, std::vector<float>>
parse_key_val_arg(const std::string& arg, unsigned expected_num_vals_per_key) {
    // Verify the format of the string. It must have one and only one colon.
    unsigned colon_count = 0;
    for (char c : arg) {
        if (c == ':')
            colon_count++;
    }
    if (colon_count != 1) {
        VTR_LOG_ERROR("Invalid argument string: %s\n",
                      arg.c_str());
        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "Error when parsing argument string");
    }

    // Split the string along the colon.
    auto del_pos = arg.find(':');
    std::string key_regex_str = arg.substr(0, del_pos);
    std::string val_list_str = arg.substr(del_pos + 1, std::string::npos);

    // Verify that there are a correct number of commas given the expected number
    // of values.
    unsigned comma_count = 0;
    for (char c : val_list_str) {
        if (c == ',')
            comma_count++;
    }
    if (comma_count != expected_num_vals_per_key - 1) {
        VTR_LOG_ERROR("Invalid argument string (too many commas): %s\n",
                      arg.c_str());
        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "Error when parsing argument string");
    }

    // Collect the comma seperated values into a vector.
    std::vector<float> vals;
    vals.reserve(expected_num_vals_per_key);

    // As we are reading each comma-seperated value, keep track of the current
    // part of the string we are reading. We read from left to right.
    std::string acc_val_list_str = val_list_str;

    // For each expected value up to the last one, parse the current value before
    // the comma.
    VTR_ASSERT(expected_num_vals_per_key > 0);
    for (unsigned i = 0; i < expected_num_vals_per_key - 1; i++) {
        // Split the string before and after the comma.
        auto comma_pos = acc_val_list_str.find(",");
        VTR_ASSERT(comma_pos != std::string::npos);
        std::string current_val_str = val_list_str.substr(0, comma_pos);
        // Send the string after the comma to the next iteration.
        acc_val_list_str = val_list_str.substr(comma_pos + 1, std::string::npos);

        // Cast the string before the comma into a float and store it.
        float current_val = str_to_float_or_error(current_val_str);
        vals.push_back(current_val);
    }

    // Parse the last value in the list. This one should not have a comma in it.
    VTR_ASSERT(acc_val_list_str.find(",") == std::string::npos);
    float last_val = str_to_float_or_error(acc_val_list_str);
    vals.push_back(last_val);

    // Return the results as a tuple.
    return std::make_tuple(key_regex_str, vals);
}

static float str_to_float_or_error(const std::string& str) {
    float val = -1;
    try {
        val = std::stof(str);
    } catch (const std::invalid_argument& e) {
        VTR_LOG_ERROR("Error while parsing float arg value: %s\n"
                      "Failed with invalid argument: %s\n",
                      str.c_str(),
                      e.what());
    } catch (const std::out_of_range& e) {
        VTR_LOG_ERROR("Error while parsing float arg value: %s\n"
                      "Failed with out of range: %s\n",
                      str.c_str(),
                      e.what());
    }
    return val;
}
