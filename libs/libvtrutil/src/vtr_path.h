#ifndef VTR_PATH_H
#define VTR_PATH_H
#include <string>
#include <array>

namespace vtr {

//Splits off the name and extension (including ".") of the specified filename
std::array<std::string, 2> split_ext(const std::string& filename);

//Returns the basename of path (i.e. the last filename component)
//  For example, the path "/home/user/my_files/test.blif" -> "test.blif"
std::string basename(const std::string& path);

//Returns the dirname of path (i.e. everything except the last filename component)
//  For example, the path "/home/user/my_files/test.blif" -> "/home/user/my_files/"
std::string dirname(const std::string& path);

//Returns the current working directory
std::string getcwd();

} // namespace vtr
#endif
