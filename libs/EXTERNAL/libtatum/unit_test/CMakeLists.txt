#
# Tatum Unit Tests (Catch2)
#
file(GLOB_RECURSE TEST_SOURCES *.cpp)
add_executable(test_tatum ${TEST_SOURCES})
target_link_libraries(test_tatum
                        libtatum
                        Catch2::Catch2WithMain)

add_test(NAME test_tatum
    COMMAND test_tatum
    --colour-mode ansi
    )
