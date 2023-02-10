# Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

PLUGIN_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

ODIN_II_DIR=src

NAME = parmys
SOURCES = parmys.cc \
		  parmys_arch.cc \
		  parmys_update.cc \
		  parmys_utils.cc \
		  parmys_resolve.cc \
		  ${ODIN_II_DIR}/adder.cc \
		  ${ODIN_II_DIR}/enum_str.cc \
		  ${ODIN_II_DIR}/mixing_optimization.cc \
		  ${ODIN_II_DIR}/read_xml_config_file.cc \
		  ${ODIN_II_DIR}/odin_error.cc \
		  ${ODIN_II_DIR}/odin_util.cc \
		  ${ODIN_II_DIR}/netlist_statistic.cc \
		  ${ODIN_II_DIR}/netlist_utils.cc \
		  ${ODIN_II_DIR}/netlist_cleanup.cc \
		  ${ODIN_II_DIR}/node_utils.cc \
		  ${ODIN_II_DIR}/multiplier.cc \
		  ${ODIN_II_DIR}/subtractor.cc \
		  ${ODIN_II_DIR}/hard_soft_logic_mixer.cc \
		  ${ODIN_II_DIR}/odin_ii.cc \
		  ${ODIN_II_DIR}/string_cache.cc \
		  ${ODIN_II_DIR}/partial_map.cc \
		  ${ODIN_II_DIR}/hard_block.cc \
		  ${ODIN_II_DIR}/block_memory.cc \
		  ${ODIN_II_DIR}/memory.cc \
		  ${ODIN_II_DIR}/netlist_visualizer.cc \
		  ${ODIN_II_DIR}/hash_table.cc \
		  ${ODIN_II_DIR}/ast_util.cc

VTR_INSTALL_DIR ?= /usr/local

include ../Makefile_plugin.common

CXXFLAGS += -std=c++14 -Wall -W -Wextra \
            -Wno-deprecated-declarations \
            -Wno-unused-parameter \
			-I${ODIN_II_DIR}/include \
			-I${VTR_INSTALL_DIR}/include/libarchfpga \
			-I${VTR_INSTALL_DIR}/include/liblog \
			-I${VTR_INSTALL_DIR}/include/libpugiutil \
			-I${VTR_INSTALL_DIR}/include/libpugixml \
			-I${VTR_INSTALL_DIR}/include/librtlnumber \
			-I${VTR_INSTALL_DIR}/include/libvtrutil

LDFLAGS += -L${VTR_INSTALL_DIR}/lib \
		   -L${VTR_INSTALL_DIR}/bin

LDLIBS += -larchfpga \
		  -lvtrutil \
		  -llog \
		  -lpugixml \
		  -lpugiutil \
		  -lrtlnumber \
		  -lpthread

TECHLIBS_DIR = techlibs
VERILOG_MODULES = adff2dff.v \
				  adffe2dff.v \
				  aldff2dff.v \
				  aldffe2dff.v \
				  vtr_primitives.v

install_modules:
	$(foreach f, $(wildcard $(TECHLIBS_DIR)/*), install -D $(f) $(YOSYS_DATA_DIR)/parmys/$(notdir $(f));)

install: install_modules

clean_modules:
	rm -rf ./third_party

clean: clean_modules