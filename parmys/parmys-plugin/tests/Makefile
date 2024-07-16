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

TESTS = raygentop \
        eltwise_layer \
		VexRiscv_Lite \
		mips32r1_core
        
include $(shell pwd)/../../Makefile_test.common

raygentop_verify = true
eltwise_layer_verify = true
VexRiscv_Lite_verify = true
mips32r1_core_verify = true

clean_modules:
	@find . -name "*.net.dot" -or -name "*.yosys.blif" -or -name "*.info" | xargs rm -rf

clean: clean_modules
