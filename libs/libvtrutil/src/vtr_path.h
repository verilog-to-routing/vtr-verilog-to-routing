#pragma once

#include <string>
#include <array>

/**
 * @file 
 * @brief This file defines some useful utilities to handle paths
 */
namespace vtr {

///@brief Splits off the name and extension (including ".") of the specified filename
std::array<std::string, 2> split_ext(const std::string& filename);

/**
 * @brief Returns the basename of path (i.e. the last filename component)
 *
 *  For example, the path "/home/user/my_files/test.blif" -> "test.blif"
 */
std::string basename(const std::string& path);

/**
 * Returns the dirname of path (i.e. everything except the last filename component)
 *
 *  For example, the path "/home/user/my_files/test.blif" -> "/home/user/my_files/"
 */
std::string dirname(const std::string& path);

///@brief Returns the current working directory
std::string getcwd();

} // namespace vtr
