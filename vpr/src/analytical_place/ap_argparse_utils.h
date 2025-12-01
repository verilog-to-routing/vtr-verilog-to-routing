#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    June 2025
 * @brief   Declarations of utility functions used to parse AP options.
 */

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Parser method for parsing a list of arguments of the form:
 *      "<key_regex>:<val1>,<val2>,<val3>,..."
 *
 * This method will will return a map containing the value for each key matched.
 * The map will not contain an entry for a key that was not set by the arguments.
 *
 * Example usage:
 *      // Create a list of valid keys.
 *      std::vector<std::string> valid_keys = {"foo", "bar"}
 *
 *      // User passed regex args. Sets all values to {0.5, 0.5, 0.5} THEN sets
 *      // "foo" specifically to {0.1, 0.2, 0.3}.
 *      // NOTE: Arguments are read left to right (first to last).
 *      std::vector<std::string> arg_vals = {".*:0.5,0.5,0.5", "foo:0.1,0.2,0.3"}
 *
 *      auto key_to_val_map = key_to_float_argument_parser(arg_vals,
 *                                                         valid_keys,
 *                                                         3);
 *      // Map will contain {0.1, 0.2, 0.3} for "foo" and {0.5, 0.5, 0.5} for "bar"
 *
 *  @param arg_vals
 *      The list of arguments to parse.
 *  @param valid_keys
 *      A list of valid keys that the argument regex patterns can match for.
 *  @param expected_num_vals_per_key
 *      The expected number of floating point values per key.
 */
std::unordered_map<std::string, std::vector<float>>
key_to_float_argument_parser(const std::vector<std::string>& arg_vals,
                             const std::vector<std::string>& valid_keys,
                             unsigned expected_num_vals_per_key = 1);
