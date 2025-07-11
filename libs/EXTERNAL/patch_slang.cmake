# Patch step for yosys-slang
#
# The variable 'IN' points to vtr_root/libs/EXTERNAL/yosys-slang/src/slang_frontend.cc
# This file contains the UndrivenPass that we want to disable
#
# This patch step finds the line where the UndrivenPass is called and comments out that line
#
# The UndrivenPass needs to be disabled due to unsupported synchronous rules in vtr_primitives.v

if(NOT DEFINED IN)
	message(FATAL_ERROR "patch_slang.cmake: IN (SLANG_FE) variable not set.")
endif()
file(READ "${IN}" SLANG_FRONTEND_CONTENTS)
string(REPLACE "call(design, \"undriven\");" "// call(design, \"undriven\");" SLANG_PATCHED "${SLANG_FRONTEND_CONTENTS}")
        if(NOT SLANG_FRONTEND_CONTENTS STREQUAL SLANG_PATCHED)
            message(STATUS "Patching slang_frontend.cc to disable UndrivenPass")
            file(WRITE "${IN}" "${SLANG_PATCHED}")
        endif()
