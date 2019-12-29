# Helper macro for creating convenient targets
find_program(GDB_PATH gdb)

# Adds -run and -dbg targets
macro(addRunAndDebugTargets TARGET)
    add_custom_target(${TARGET}-run
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL
        DEPENDS ${TARGET}
        COMMAND ./${TARGET})

    # convenience run gdb target
    if(GDB_PATH)
        add_custom_target(${TARGET}-gdb
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            USES_TERMINAL
            DEPENDS ${TARGET}
            COMMAND ${GDB_PATH} ./${TARGET})
    endif()
endmacro()

#------------------------------------------------------------------------------
# Usefull for adding header only libraries
# Example usage:
#
#     ExternalHeaderOnly_Add("Catch"
#         "https://github.com/catchorg/Catch2.git" "master" "single_include/catch2")
#
# Use with:
#     target_link_libraries(unittests Catch)
# This will add the INCLUDE_FOLDER_PATH to the `unittests` target.

macro(ExternalHeaderOnly_Add LIBNAME REPOSITORY GIT_TAG INCLUDE_FOLDER_PATH)
    ExternalProject_Add(
        ${LIBNAME}_download
        PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/${LIBNAME}
        GIT_REPOSITORY ${REPOSITORY}
        # For shallow git clone (without downloading whole history)
        # GIT_SHALLOW 1
        # For point at certain tag
        GIT_TAG origin/${GIT_TAG}
        #disables auto update on every build
        UPDATE_DISCONNECTED 1
        #disable following
        CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_DIR "" INSTALL_COMMAND ""
        )
    # special target
    add_custom_target(${LIBNAME}_update
        COMMENT "Updated ${LIBNAME}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${LIBNAME}/src/${LIBNAME}_download
        COMMAND ${GIT_EXECUTABLE} fetch --recurse-submodules
        COMMAND ${GIT_EXECUTABLE} reset --hard origin/${GIT_TAG}
        COMMAND ${GIT_EXECUTABLE} submodule update --init --force --recursive --remote --merge
        DEPENDS ${LIBNAME}_download)

    set(${LIBNAME}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${LIBNAME}/src/${LIBNAME}_download/)
    add_library(${LIBNAME} INTERFACE)
    add_dependencies(${LIBNAME} ${LIBNAME}_download)
    add_dependencies(update ${LIBNAME}_update)
    target_include_directories(${LIBNAME} SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${LIBNAME}/src/${LIBNAME}_download/${INCLUDE_FOLDER_PATH})
endmacro()

#------------------------------------------------------------------------------
# This command will clone git repo during cmake setup phase, also adds 
# ${LIBNAME}_update target into general update target.
# Example usage:
#
#   ExternalDownloadNowGit(cpr https://github.com/finkandreas/cpr.git origin/master)
#   add_subdirectory(${cpr_SOURCE_DIR})
#

macro(ExternalDownloadNowGit LIBNAME REPOSITORY GIT_TAG)

    set(${LIBNAME}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${LIBNAME}/src/${LIBNAME}_download/)

    # clone repository if not done
    if(IS_DIRECTORY ${${LIBNAME}_SOURCE_DIR})
        message(STATUS "Already downloaded: ${REPOSITORY}")
    else()
        message(STATUS "Clonning: ${REPOSITORY}")
        execute_process(
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${GIT_EXECUTABLE} clone --recursive ${REPOSITORY} ${LIBNAME}/src/${LIBNAME}_download
            )
        # switch to target TAG and update submodules
        execute_process(
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${LIBNAME}/src/${LIBNAME}_download
            COMMAND ${GIT_EXECUTABLE} reset --hard origin/${GIT_TAG}
            COMMAND ${GIT_EXECUTABLE} submodule update --init --force --recursive --remote --merge
            )
    endif()

    # special update target
    add_custom_target(${LIBNAME}_update
        COMMENT "Updated ${LIBNAME}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${LIBNAME}/src/${LIBNAME}_download
        COMMAND ${GIT_EXECUTABLE} fetch --recurse-submodules
        COMMAND ${GIT_EXECUTABLE} reset --hard origin/${GIT_TAG}
        COMMAND ${GIT_EXECUTABLE} submodule update --init --force --recursive --remote --merge)
    # Add this as dependency to the general update target
    add_dependencies(update ${LIBNAME}_update)
endmacro()

#------------------------------------------------------------------------------
# Other MISC targets - formating, static analysis
# format, cppcheck, tidy
macro(addMiscTargets)
    file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.cc *.c)
    file(GLOB_RECURSE ALL_HEADER_FILES *.h *.hpp)

    # Static analysis via clang-tidy target
    # We check for program, since when it is not here, target makes no sense
    find_program(TIDY_PATH clang-tidy PATHS /usr/local/Cellar/llvm/*/bin)
    if(TIDY_PATH)
        message(STATUS "clang-tidy - static analysis              YES ")
        add_custom_target(tidy
            COMMAND ${TIDY_PATH} -header-filter=.* ${ALL_SOURCE_FILES} -p=./ )
    else()
        message(STATUS "clang-tidy - static analysis              NO ")
    endif()

    # cpp check static analysis
    find_program(CPPCHECK_PATH cppcheck)
    if(CPPCHECK_PATH)
        message(STATUS "cppcheck - static analysis                YES ")
        add_custom_target(
                cppcheck
                COMMAND ${CPPCHECK_PATH}
                --enable=warning,performance,portability,information,missingInclude
                --std=c++11
                --template=gcc
                --verbose
                --quiet
                ${ALL_SOURCE_FILES}
        )
    else()
        message(STATUS "cppcheck - static analysis                NO ")
    endif()

    # run clang-format on all files
    find_program(FORMAT_PATH clang-format)
    if(FORMAT_PATH)
        message(STATUS "clang-format - code formating             YES ")
        add_custom_target(format
            COMMAND ${FORMAT_PATH} -i ${ALL_SOURCE_FILES} ${ALL_HEADER_FILES} )
    else()
        message(STATUS "clang-format - code formating             NO ")
    endif()


    # Does not work well, left here for future work, but it would still only
    # provides same info as tidy, only in html form.
    #
    # Produces html analysis in *.plist dirs in build dir or build/source directory
    # add_custom_target(
    #     analyze
    #     COMMAND rm -rf ../*.plist
    #     COMMAND rm -rf *.plist
    #     COMMAND clang-check -analyze -extra-arg -Xclang -extra-arg -analyzer-output=html
    #     ${ALL_SOURCE_FILES}
    #     -p=./
    #     COMMAND echo ""
    #     )
endmacro()

#------------------------------------------------------------------------------
# Force compilers, this was deprecated in CMake, but still comes handy sometimes
macro(FORCE_C_COMPILER compiler id)
    set(CMAKE_C_COMPILER "${compiler}")
    set(CMAKE_C_COMPILER_ID_RUN TRUE)
    set(CMAKE_C_COMPILER_ID ${id})
    set(CMAKE_C_COMPILER_FORCED TRUE)

    # Set old compiler id variables.
    if(CMAKE_C_COMPILER_ID MATCHES "GNU")
        set(CMAKE_COMPILER_IS_GNUCC 1)
    endif()
endmacro()

macro(FORCE_CXX_COMPILER compiler id)
    set(CMAKE_CXX_COMPILER "${compiler}")
    set(CMAKE_CXX_COMPILER_ID_RUN TRUE)
    set(CMAKE_CXX_COMPILER_ID ${id})
    set(CMAKE_CXX_COMPILER_FORCED TRUE)

    # Set old compiler id variables.
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        set(CMAKE_COMPILER_IS_GNUCXX 1)
    endif()
endmacro()
