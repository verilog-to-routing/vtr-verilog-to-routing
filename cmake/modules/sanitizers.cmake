
#------------------------------------------------------------------------------
# Clang and gcc sanitizers
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message(STATUS "Sanitizers:")

    option(ADDRESS_SANITIZER "description" OFF)
    message(STATUS "  + ADDRESS_SANITIZER                     ${ADDRESS_SANITIZER}")
    if(ADDRESS_SANITIZER)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        link_libraries(-fsanitize=address -fno-omit-frame-pointer)
    endif()

    option(UB_SANITIZER "description" OFF)
    message(STATUS "  + UB_SANITIZER                          ${UB_SANITIZER}")
    if(UB_SANITIZER)
        add_compile_options(-fsanitize=undefined)
        link_libraries(-fsanitize=undefined)
    endif()

    option(THREAD_SANITIZER "description" OFF)
    message(STATUS "  + THREAD_SANITIZER                      ${THREAD_SANITIZER}")
    if(THREAD_SANITIZER)
        add_compile_options(-fsanitize=undefined)
        link_libraries(-fsanitize=undefined)
    endif()

    # Clang only
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        option(MEMORY_SANITIZER "description" OFF)
        message(STATUS "  + MEMORY_SANITIZER                      ${MEMORY_SANITIZER}")
        if(MEMORY_SANITIZER)
            add_compile_options(-fsanitize=memory -fno-omit-frame-pointer)
            link_libraries(-fsanitize=memory -fno-omit-frame-pointer)
        endif()
    endif()
endif()

