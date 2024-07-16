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

PLUGIN_LIST := parmys
PLUGINS := $(foreach plugin,$(PLUGIN_LIST),$(plugin).so)
PLUGINS_INSTALL := $(foreach plugin,$(PLUGIN_LIST),install_$(plugin))
PLUGINS_CLEAN := $(foreach plugin,$(PLUGIN_LIST),clean_$(plugin))
PLUGINS_TEST := $(foreach plugin,$(PLUGIN_LIST),test_$(plugin))

.PHONY: all
all: plugins

TOP_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
REQUIREMENTS_FILE ?= requirements.txt
ENVIRONMENT_FILE ?= environment.yml

-include third_party/make-env/conda.mk

define install_plugin =
.PHONY: $(1).so
$(1).so:
	$$(MAKE) -C $(1)-plugin $$@

.PHONY: install_$(1)
install_$(1):
	$$(MAKE) -C $(1)-plugin install

.PHONY: clean_$(1)
clean_$(1):
	$$(MAKE) -C $(1)-plugin clean

.PHONY: test_$(1)
test_$(1):
	@$$(MAKE) --no-print-directory -C $(1)-plugin test
endef

$(foreach plugin,$(PLUGIN_LIST),$(eval $(call install_plugin,$(plugin))))

pmgen.py:
	wget -nc -O $@ https://raw.githubusercontent.com/YosysHQ/yosys/master/passes/pmgen/pmgen.py

.PHONY: plugins
plugins: $(PLUGINS)

.PHONY: install
install: $(PLUGINS_INSTALL)

.PHONY: test
test: $(PLUGINS_TEST)

.PHONY: plugins_clean
plugins_clean: $(PLUGINS_CLEAN)

.PHONY: clean
clean:: plugins_clean
	rm -rf pmgen.py

CLANG_FORMAT ?= clang-format-8
.PHONY: format
format:
	find . \( -name "*.h" -o -name "*.cc" \) -and -not -path '*/third_party/*' -print0 | xargs -0 -P $$(nproc) ${CLANG_FORMAT} -style=file -i

VERIBLE_FORMAT ?= verible-verilog-format
.PHONY: format-verilog
format-verilog:
	find */tests \( -name "*.v" -o -name "*.sv" \) -and -not -path '*/third_party/*' -print0 | xargs -0 $(VERIBLE_FORMAT) --inplace
