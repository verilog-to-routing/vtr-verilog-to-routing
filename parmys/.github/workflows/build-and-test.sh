#! /bin/bash
# Copyright 2020-2022 F4PGA Authors
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

set -e

source .github/workflows/common.sh

if [ -z "${PLUGIN_NAME}" ]; then
	echo "Missing \${PLUGIN_NAME} env value"
	exit 1
fi

##########################################################################

start_section Building

if [ "$PLUGIN_NAME" == "xdc" ] || [ "$PLUGIN_NAME" == "sdc" ]; then 
    make design_introspection.so -j`nproc`
	make install_design_introspection -j`nproc`
fi 

export CXXFLAGS=-Werror
make UHDM_INSTALL_DIR=`pwd`/env/conda/envs/yosys-plugins/ VTR_INSTALL_DIR=`pwd`/env/conda/envs/yosys-plugins ${PLUGIN_NAME}.so -j`nproc`
unset CXXFLAGS

end_section

##########################################################################

start_section Installing
make install_${PLUGIN_NAME} -j`nproc`
end_section

##########################################################################

start_section Testing
make test_${PLUGIN_NAME} -j`nproc`
end_section

##########################################################################

start_section Cleanup
make clean_${PLUGIN_NAME} -j`nproc`
end_section

##########################################################################
