# Use the black python formatter on Python files in the root directory,
# as well those in DIRS_TO_FORMAT_PY, recursively
add_custom_target(format-py 
    COMMAND find ${PROJECT_SOURCE_DIR} -maxdepth 1 -iname \"*.py\" -print0 | xargs -0 -P 0 -n 1 black -l 100
    COMMAND find ${DIRS_TO_FORMAT_PY} -iname \"*.py\" -print0 | xargs -0 -P 0 -n 1 black -l 100
)
