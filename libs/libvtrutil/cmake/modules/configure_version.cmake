#
# Versioning information
#
#Figure out the git revision
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --long --dirty
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    OUTPUT_VARIABLE VTR_VCS_REVISION
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE GIT_DESCRIBE_RETURN_VALUE)

    if(NOT GIT_DESCRIBE_RETURN_VALUE EQUAL 0)
        #Git describe failed, usually this means we
        #aren't in a git repo -- so don't set a VCS 
        #revision
        set(VTR_VCS_REVISION "unkown")
    endif()

    #Call again with exclude to get the revision excluding any tags
    #(i.e. just the commit ID and dirty flag)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --long --dirty --exclude '*'
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    OUTPUT_VARIABLE VTR_VCS_REVISION_SHORT
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE GIT_DESCRIBE_RETURN_VALUE)

    if(NOT GIT_DESCRIBE_RETURN_VALUE EQUAL 0)
        #Git describe failed, usually this means we
        #aren't in a git repo -- so don't set a VCS 
        #revision
        set(VTR_VCS_REVISION_SHORT "unkown")
    endif()
else()
    #Couldn't find git, so can't look-up VCS revision
    set(VTR_VCS_REVISION "unkown")
    set(VTR_VCS_REVISION_SHORT "unkown")
endif()


#Set the version according to semver.org
set(VTR_VERSION "${VTR_VERSION_MAJOR}.${VTR_VERSION_MINOR}.${VTR_VERSION_PATCH}")
if(VTR_VERSION_PRERELEASE)
    set(VTR_VERSION "${VTR_VERSION}-${VTR_VERSION_PRERELEASE}")
endif()
set(VTR_VERSION_SHORT ${VTR_VERSION})
if(VTR_VCS_REVISION)
    set(VTR_VERSION "${VTR_VERSION}+${VTR_VCS_REVISION_SHORT}")
endif()

#Other build meta-data
string(TIMESTAMP VTR_BUILD_TIMESTAMP)
set(VTR_BUILD_TIMESTAMP "${VTR_BUILD_TIMESTAMP}")
set(VTR_BUILD_INFO "${VTR_BUILD_INFO}")

message(STATUS "VTR Version: ${VTR_VERSION}")

configure_file(${IN_FILE} ${OUT_FILE})
