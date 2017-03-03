#
# Versioning information
#
#Figure out the git revision
find_package(Git QUIET)
if(GIT_FOUND)
    exec_program(${GIT_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}
                 ARGS describe --always --long --dirty
                 OUTPUT_VARIABLE VTR_VCS_REVISION)
else()
    set(VTR_VCS_REVISION "")
endif()


#Set the version according to semver.org
set(VTR_VERSION "${VTR_VERSION_MAJOR}.${VTR_VERSION_MINOR}.${VTR_VERSION_PATCH}")
if(VTR_VERSION_PRERELEASE)
    set(VTR_VERSION "${VTR_VERSION}-${VTR_VERSION_PRERELEASE}")
endif()
set(VTR_VERSION_SHORT ${VTR_VERSION})
if(VTR_VCS_REVISION)
    set(VTR_VERSION "${VTR_VERSION}+${VTR_VCS_REVISION}")
endif()

#Other build meta-data
string(TIMESTAMP VTR_BUILD_TIMESTAMP)

message(STATUS "VTR Version: ${VTR_VERSION}")

configure_file(${IN_FILE} ${OUT_FILE})
