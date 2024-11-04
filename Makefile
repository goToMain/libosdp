#
#  Copyright (c) 2021-2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

ifeq ($(wildcard config.make),)
$(error run ./configure.sh first. See ./configure.sh -h)
endif

include config.make

O ?= $(BUILD_DIR)
OBJ_LIBOSDP := $(SRC_LIBOSDP:%.c=$(O)/%.o)
OBJ_TEST := $(SRC_TEST:%.c=$(O)/%.o)
CCFLAGS += -Wall -Wextra -O3

ifeq ($(V),)
MAKE := make -s
Q := @
else
Q :=
MAKE := make
endif

.PHONY: all
all: libosdp $(TARGETS)

.PHONY: libosdp
libosdp: $(O)/libosdp.a $(O)/libosdp.pc

.PHONY: pd_app
pd_app: $(O)/pd_app.elf

.PHONY: cp_app
cp_app: $(O)/cp_app.elf

$(O)/%.o: %.c
	@echo "  CC $<"
	@mkdir -p $(@D)
	$(Q)$(CC) -c $< $(CCFLAGS) $(CCFLAGS_EXTRA) -o $@

$(O)/libosdp.a: CCFLAGS_EXTRA=-Iutils/include -Iinclude -Isrc -I$(O)
$(O)/libosdp.a: $(OBJ_LIBOSDP)
	@echo "  AR $(@F)"
	$(Q)$(AR) qc $@ $^

## Samples

$(O)/cp_app.elf: $(O)/libosdp.a
	@echo "LINK $(@F)"
	$(Q)$(CC) $(CCFLAGS) examples/c/cp_app.c -o $@ -Iinclude -L$(O) -losdp

$(O)/pd_app.elf: $(O)/libosdp.a
	@echo "LINK $(@F)"
	$(Q)$(CC) $(CCFLAGS) examples/c/pd_app.c -o $@ -Iinclude -L$(O) -losdp

## Tests

.PHONY: check
check: CCFLAGS_EXTRA=-DUNIT_TESTING -Iutils/include -Iinclude -Isrc -I$(O)
check: clean $(OBJ_TEST)
	@echo "LINK $@"
	$(Q)$(CC) $(CCFLAGS) $(OBJ_TEST) -o $(O)/unit-test
	$(Q)$(O)/unit-test

## Clean

.PHONY: clean
clean:
	$(Q)rm -f $(O)/src/*.o $(O)/src/crypto/*.o $(OBJ_TEST)
	$(Q)rm -f $(O)/*.a $(O)/*.elf $(O)/*.pc

.PHONY: distclean
distclean: clean
	$(Q)rm config.make
	$(Q)rm -rf $(O)

## Install

.PHONY: install
install: libosdp
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(O)/libosdp.a $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/lib/pkgconfig
	install -m 644 $(O)/libosdp.pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 644 include/osdp.h $(DESTDIR)$(PREFIX)/include/
	install -m 644 include/osdp.hpp $(DESTDIR)$(PREFIX)/include/
	install -m 644 $(O)/include/osdp_export.h $(DESTDIR)$(PREFIX)/include/

