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

# Look for location binaries first
export PATH="$HOME/.local-bin/bin:$PATH"

# OS X specific common setup
if [[ "${OS}" == "macOS" ]]; then
	export PATH="/usr/local/opt/ccache/libexec:$PATH"
fi

# Parallel builds!
MAKEFLAGS="-j 2"

function action_fold() {
	if [ "$1" = "start" ]; then
		echo "::group::$2"
		SECONDS=0
	else
		duration=$SECONDS
		echo "::endgroup::"
		printf "${GRAY}took $(($duration / 60)) min $(($duration % 60)) sec.${NC}\n"
	fi
	return 0;
}

function start_section() {
	action_fold start "$1"
	echo -e "${PURPLE}SymbiFlow Yosys Plugins${NC}: - $2${NC}"
	echo -e "${GRAY}-------------------------------------------------------------------${NC}"
}

export -f start_section

function end_section() {
	echo -e "${GRAY}-------------------------------------------------------------------${NC}"
	action_fold end "$1"
}

export -f end_section
