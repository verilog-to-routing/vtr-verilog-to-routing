# Utility functions to be used in tests.

# Return path where the test output file "filename"
# is to be written.
proc test_output_path { filename } {
    return "$::env(TEST_OUTPUT_PREFIX)${filename}"
}
