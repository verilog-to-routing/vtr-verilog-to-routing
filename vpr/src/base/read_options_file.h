#pragma once

/**
 * @file read_options_file.h
 * @brief Reads VPR command-line options from a YAML file (--options_file).
 *
 * The file is a flat YAML map from option name to value, for example:
 *
 *     architecture: k6_frac_N10_mem32K_40nm.xml
 *     circuit: my_circuit.blif
 *     route_chan_width: 100
 *     seed: 7
 *     pack: true
 *     timing_analysis: on
 *     timing_report_path_categories: [in2reg, reg2reg]
 *
 * Each entry is converted into the equivalent command-line arguments (e.g. "--seed 7"), which
 * libargparse splices into the command-line at the position of --options_file. This code therefore
 * needs no knowledge of individual options: any option VPR accepts on the command-line can be
 * given in the file, and is checked by the same code.
 */

#include <string>
#include <vector>

#include "argparse.hpp"

/**
 * @brief Converts a YAML options file into command-line argument strings.
 *
 * Each key is the name of an option, without or with its leading dashes (e.g. "seed" or "--seed"),
 * or the name of a positional argument (e.g. "circuit"). Values are converted as follows:
 *  - Flags (options which take no value): true adds the flag, false adds nothing.
 *  - Options which take one value: the value is passed on exactly as written in the file.
 *  - Options which take multiple values: a YAML sequence (or a single value).
 *  - Positional arguments: if any are given, all must be given. They are added before the
 *    options, in the order the parser expects them.
 *
 * This is used as the parser's argparse::FileExpander, so errors are reported by throwing
 * argparse::ArgParseError, which the parser reports together with the usage message.
 *
 * @param file_path Path of the YAML file.
 * @param parser    The parser the arguments are for; used to look up each option.
 * @return The argument strings, e.g. {"arch.xml", "circuit.blif", "--seed", "7", "--pack"}.
 */
std::vector<std::string> expand_yaml_options_file(const std::string& file_path,
                                                  const argparse::ArgumentParser& parser);
