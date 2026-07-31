# https://github.com/povik/yosys-slang/blob/76b83eb5b73ba871797e6db7bc5fed10af380be4/cmake/FindYosys.cmake

if(TARGET yosys::yosys)
else()
    set(YOSYS_CONFIG "yosys-config" CACHE STRING "Location of yosys-config utility")
    if(DEFINED YOSYS_TREE)
	    message(STATUS "Using yosys path: ${YOSYS_TREE}")

	    set(YOSYS_CONFIG "${YOSYS_TREE}/yosys-config")
	    set(YOSYS_BINDIR "${YOSYS_TREE}")
	    set(YOSYS_DATDIR "${YOSYS_TREE}/share")

        message(STATUS "yosys-config --bindir (override): ${YOSYS_BINDIR}")
        message(STATUS "yosys-config --datdir (override): ${YOSYS_DATDIR}")

        execute_process(
            COMMAND ${YOSYS_CONFIG} --cxxflags
            OUTPUT_VARIABLE YOSYS_CXXFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        # Prepend to ensure tree include paths are used
        string(PREPEND YOSYS_CXXFLAGS "-I${YOSYS_DATDIR}/include ")
        string(REGEX REPLACE " +" ";" YOSYS_CXXFLAGS ${YOSYS_CXXFLAGS})
        list(FILTER YOSYS_CXXFLAGS INCLUDE REGEX "^-[ID]")
        message(STATUS "yosys-config --cxxflags (filtered): ${YOSYS_CXXFLAGS}")
    elseif(DEFINED YOSYS_PATH)
            message(STATUS "Using yosys path: ${YOSYS_PATH}")

            set(YOSYS_CONFIG "${YOSYS_PATH}/bin/yosys-config")
            set(YOSYS_BINDIR "${YOSYS_PATH}/bin")
            set(YOSYS_DATDIR "${YOSYS_PATH}/share/yosys")

        message(STATUS "yosys-config --bindir (override): ${YOSYS_BINDIR}")
        message(STATUS "yosys-config --datdir (override): ${YOSYS_DATDIR}")

        execute_process(
            COMMAND ${YOSYS_CONFIG} --cxxflags
            OUTPUT_VARIABLE YOSYS_CXXFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        # Prepend to ensure tree include paths are used
        string(PREPEND YOSYS_CXXFLAGS "-I${YOSYS_DATDIR}/include ")
        string(REGEX REPLACE " +" ";" YOSYS_CXXFLAGS ${YOSYS_CXXFLAGS})
        list(FILTER YOSYS_CXXFLAGS INCLUDE REGEX "^-[ID]")
        message(STATUS "yosys-config --cxxflags (filtered): ${YOSYS_CXXFLAGS}")

    else()
        message(STATUS "Using yosys: ${YOSYS_CONFIG}")

        execute_process(
            COMMAND ${YOSYS_CONFIG} --bindir
            OUTPUT_VARIABLE YOSYS_BINDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        message(STATUS "yosys-config --bindir: ${YOSYS_BINDIR}")

        execute_process(
            COMMAND ${YOSYS_CONFIG} --datdir
            OUTPUT_VARIABLE YOSYS_DATDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        message(STATUS "yosys-config --datdir: ${YOSYS_DATDIR}")

        execute_process(
            COMMAND ${YOSYS_CONFIG} --cxxflags
            OUTPUT_VARIABLE YOSYS_CXXFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        string(REGEX REPLACE " +" ";" YOSYS_CXXFLAGS ${YOSYS_CXXFLAGS})
        list(FILTER YOSYS_CXXFLAGS INCLUDE REGEX "^-[ID]")
        message(STATUS "yosys-config --cxxflags (filtered): ${YOSYS_CXXFLAGS}")

        set(YOSYS_BINDIR ${YOSYS_BINDIR})
        set(YOSYS_DATDIR ${YOSYS_DATDIR})
    endif()

    # Extract version
    set(YOSYS_DFLAGS ${YOSYS_CXXFLAGS})
    list(FILTER YOSYS_DFLAGS INCLUDE REGEX "^-DYOSYS_VER=")
    if (${YOSYS_DFLAGS})
        list(GET YOSYS_DFLAGS 0 YOSYS_VERSION_D)
        string(REGEX MATCH "\"(.*)\"" YOSYS_VERSION_MATCH ${YOSYS_VERSION_D})
        set(YOSYS_VERSION ${CMAKE_MATCH_1})
    else()
        set(YOSYS_VERSION unknown)
    endif()
    message(STATUS "yosys version: ${YOSYS_VERSION}")

    add_library(yosys::yosys INTERFACE IMPORTED)
    target_compile_options(yosys::yosys INTERFACE ${YOSYS_CXXFLAGS})

    if(WIN32)
        execute_process(
            COMMAND ${YOSYS_CONFIG} --linkflags
            OUTPUT_VARIABLE YOSYS_LINKFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        string(REGEX REPLACE " +" ";" YOSYS_LINKFLAGS ${YOSYS_LINKFLAGS})
        list(FILTER YOSYS_LINKFLAGS INCLUDE REGEX "^-[L]")
        message(STATUS "yosys-config --linkflags (filtered): ${YOSYS_LINKFLAGS}")

        target_link_options(yosys::yosys INTERFACE ${YOSYS_LINKFLAGS})

        target_link_libraries(yosys::yosys INTERFACE yosys_exe)
    endif()

endif()
