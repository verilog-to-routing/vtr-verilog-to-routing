option(
  EZGL_BUILD_EXAMPLES
  "Build the EZGL example executables."
  ${IS_ROOT_PROJECT} #Only build examples by default if EZGL is the root cmake project
)

option(
  EZGL_BUILD_DOCS
  "Create HTML/PDF documentation (requires Doygen)."
  OFF
)
