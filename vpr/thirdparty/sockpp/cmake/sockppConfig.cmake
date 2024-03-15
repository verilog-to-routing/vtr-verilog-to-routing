# sockppConfig.cmake
#
# The following import targets may be created depending on configuration:
#
#   Sockpp::sockpp
#   Sockpp::sockpp-static
#

include(CMakeFindDependencyMacro)

if(NOT TARGET Sockpp::sockpp AND NOT TARGET Sockpp::sockpp-static)
	include("${CMAKE_CURRENT_LIST_DIR}/sockppTargets.cmake")
endif()

