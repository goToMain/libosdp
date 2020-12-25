#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

ifndef VERBOSE
  MAKE=make -s
else
  MAKE=make
endif

.PHONY: default
default: build
	@$(MAKE) -C build all

.PHONY: clean
clean: build
	@$(MAKE) -C build clean

.PHONY: distclean
distclean:
	rm -rf build

.PHONY: install
install: default
	@$(MAKE) -C build install

build:
	@mkdir -p build
	@cd build && cmake ..

.PHONY: gentags
gentags:
	@rm -f tags cscope.out
	@git ls-files --recurse-submodules | \
		grep -v 'cmake/\|misc\|samples/\|doc/\|.github/\|.md$\|.txt$\|.yml$\|.cfg$\|LICENSE\|.gitignore\|.editorconfig\|CHANGELOG\|.gitmodules' | \
		ctags --tag-relative=yes -L -
	@cscope -Rb
