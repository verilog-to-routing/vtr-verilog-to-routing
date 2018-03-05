#!/bin/bash

source .travis/common.sh
set -e

$SPACER

start_section "vtr.build" "${GREEN}Building..${NC}"
make -j2
end_section "vtr.build"

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

start_section "vtr.test.1" "${GREEN}Testing..${NC} ${CYAN}vtr_reg_basic${NC}"
./run_reg_test.pl vtr_reg_basic
end_section "vtr.test.1"

$SPACER

#start_section "vtr.test.2" "${GREEN}Testing..${NC} ${CYAN}vtr_reg_strong${NC}"
#./run_reg_test.pl vtr_reg_strong -j2
#end_section "vtr.test.2"
#
#$SPACER
