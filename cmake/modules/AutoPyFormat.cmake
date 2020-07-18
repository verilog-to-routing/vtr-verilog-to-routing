# Use the black python formatter on the entire repo
#
# While black can perform it's own recursive search, it follows symlinks
# which seems to be a problem with the Travis CI build
add_custom_target(format-py 
    COMMAND find ${PROJECT_SOURCE_DIR} -iname \"*.py\" -print0 | xargs -0 -P 0 -n 1 black 
)
