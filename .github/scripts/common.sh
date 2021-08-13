eval "${MATRIX_EVAL}"
export "CC=$CC"
export "CXX=$CXX"

# Some colors, use it like following;
# echo -e "Hello ${YELLOW}yellow${NC}"
GRAY='\033[0;30m'
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

SPACER="echo -e ${GRAY} - ${NC}"

function start_section() {
	echo -e "${PURPLE}Verilog To Routing${NC}: - $2${NC}"
	echo -e "${GRAY}-------------------------------------------------------------------${NC}"
}

function end_section() {
	echo -e "${GRAY}-------------------------------------------------------------------${NC}"
}

export PREFIX=$HOME/vtr
export CMAKE_INSTALL_PREFIX_PARAMS="-DCMAKE_INSTALL_PREFIX=$PREFIX"
export BUILD_DIR=$HOME/vtr-build
