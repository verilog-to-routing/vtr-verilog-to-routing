#!/bin/bash

source .github/travis/common.sh
set -e

$SPACER

start_section "vtr.build" "${GREEN}Building..${NC}"
export FAILURE=0
make -k CMAKE_PARAMS="-DVTR_ASSERT_LEVEL=3 -DWITH_BLIFEXPLORER=on" -j2 || export FAILURE=1
end_section "vtr.build"

# When the build fails, produce the failure output in a clear way
if [ $FAILURE -ne 0 ]; then
	start_section "vtr.failure" "${RED}Build failure output..${NC}"
	make CMAKE_PARAMS="-DVTR_ASSERT_LEVEL=3 -DWITH_BLIFEXPLORER=on" -j1
	end_section "vtr.failure"
	exit 1
fi

$SPACER

start_section "vtr.build" "${GREEN}Installing..${NC}"
make install
mkdir $PREFIX/lib
mv $PREFIX/bin/*.a $PREFIX/lib/
end_section "vtr.build"

$SPACER

start_section "vtr.du" "${GREEN}Disk usage..${NC}"
du -h $PREFIX
find $PREFIX | sort && ldd $PREFIX/bin/vpr

end_section "vtr.du"

$SPACER
