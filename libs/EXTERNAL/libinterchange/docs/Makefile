# Copyright (C) 2017-2021  The SymbiFlow Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
#
# Minimal makefile for Sphinx documentation


SHELL = /bin/bash
MAKEDIR := $(dir $(lastword $(MAKEFILE_LIST)))

# You can set these variables from the command line.
SPHINXOPTS      =
SPHINXBUILD     = [ -e env/bin/activate ] && source env/bin/activate; sphinx-build
SPHINXAUTOBUILD = [ -e env/bin/activate ] && source env/bin/activate; sphinx-autobuild
SPHINXPROJ      = SymbiFlowFIF
SOURCEDIR       = .
BUILDDIR        = _build
OSFLAG          =
UNAME_S := $(shell uname -s)
ifneq (, $(findstring Linux, $(UNAME_S)))
	OSFLAG := Linux
endif
ifeq ($(UNAME_S), Darwin)
	OSFLAG := MacOSX
endif
ifneq (, $(findstring Cygwin, $(UNAME_S)))
	OSFLAG := Linux
endif
ifneq (, $(findstring MINGW, $(UNAME_S)))
	OSFLAG := Linux
endif


# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

livehtml:
	@$(SPHINXAUTOBUILD) -b html --ignore \*.swp --ignore \*~ $(SPHINXOPTS) "$(SOURCEDIR)" "$(BUILDDIR)/html"

.PHONY: help livehtml Makefile


env/Miniconda3-latest-$(OSFLAG)-x86_64.sh:
	mkdir env
	wget https://repo.anaconda.com/miniconda/Miniconda3-latest-$(OSFLAG)-x86_64.sh -O env/Miniconda3-latest-$(OSFLAG)-x86_64.sh
	chmod a+x env/Miniconda3-latest-$(OSFLAG)-x86_64.sh

env:
	rm -rf env
	make env/Miniconda3-latest-$(OSFLAG)-x86_64.sh
	./env/Miniconda3-latest-$(OSFLAG)-x86_64.sh -p $(PWD)/env -b -f
	source env/bin/activate; conda config --system --add envs_dirs $(PWD)/env/envs
	source env/bin/activate; conda config --system --add pkgs_dirs $(PWD)/env/pkgs
	source env/bin/activate; conda env update --name base --file ./environment.yml

.PHONY: env

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%:
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
