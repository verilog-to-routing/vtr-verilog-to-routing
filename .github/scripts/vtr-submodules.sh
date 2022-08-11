#!/bin/bash

echo
echo "========================================"
echo "VtR Submodules Update"
echo "----------------------------------------"

# Update GitHub submodules of Yosys plugins
if { [ $VTR_TEST == "vtr_reg_yosys" ] || [ $VTR_TEST == "vtr_reg_yosys_odin" ]; }; then
	make update_submodules
fi