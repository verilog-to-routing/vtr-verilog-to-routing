#!/bin/bash

source .github/travis/common.sh
set -e

$SPACER

start_section "vtr.setup" "${GREEN}Setting up..${NC}"

# Cleanup any cached build artifacts
echo "Build Stage Name: $TRAVIS_BUILD_STAGE_NAME"
case "$TRAVIS_BUILD_STAGE_NAME" in
	Build*)
		echo "Cleaning up any previous builds"
		rm -rf $BUILD_DIR
		rm -rf $PREFIX
		echo "--"
		echo "Using linker"
		ld --version
		gold --version
		echo "--"
		echo "Using compiler C compiler '${CC}'"
		${CC} --version
		echo "--"
		echo "Using compiler C++ compiler '${CXX}'"
		${CXX} --version
		echo "--"
		;;
	*)
		;;
esac

make version

end_section "vtr.setup"

start_section "vtr.ccache.1" "${GREEN}ccache info..${NC}"
ccache --show-stats
ccache --zero-stats
end_section "vtr.ccache.1"

$SPACER
