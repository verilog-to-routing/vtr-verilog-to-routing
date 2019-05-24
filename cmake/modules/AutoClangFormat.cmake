include(ProcessorCount)
ProcessorCount(CPU_COUNT)
if(CPU_COUNT EQUAL 0)
    message(FATAL_ERROR "Unknown number of CPUs!?")
endif()

#
# ----------------------- Code format --------------------------
#

set(FIND_TO_FORMAT_CPP
    -name '*.cpp' -print -o -name '*.h' -print
    -o -name '*.tpp' -print -o -name '*.hpp' -print)

list(APPEND DIRS_TO_FORMAT_CPP "")

#
# List the files which will be auto-formatted.
#
add_custom_target(format-cpp-files
    COMMAND find ${DIRS_TO_FORMAT_CPP} ${FIND_TO_FORMAT_CPP})

#
# Use clang-format-7 for code format
#
add_custom_target(format-cpp
    COMMAND find ${DIRS_TO_FORMAT_CPP} ${FIND_TO_FORMAT_CPP} |
    xargs -P ${CPU_COUNT} clang-format-7 -style=file -i)

#
# Use simple python script for fixing C like boxed comments
#
add_custom_target(format-cpp-fix-comments DEPENDS format-cpp
    COMMAND find ${DIRS_TO_FORMAT_CPP} ${FIND_TO_FORMAT_CPP} |
        xargs -L 1 -P ${CPU_COUNT}
        python3 ${PROJECT_SOURCE_DIR}/dev/code_format_fixup.py --inplace --fix-comments --input)

#
# Use simple python script for fixing template brackets e.g. <<>
#
add_custom_target(format-cpp-fix-template-operators DEPENDS format-cpp
    COMMAND find ${DIRS_TO_FORMAT_CPP} ${FIND_TO_FORMAT_CPP} |
        xargs -L 1 -P ${CPU_COUNT}
        python3 ${PROJECT_SOURCE_DIR}/dev/code_format_fixup.py --inplace --fix-template-operators --input)

add_custom_target(format DEPENDS format-cpp-fix-comments format-cpp-fix-template-operators)
