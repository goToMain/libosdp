#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

ifeq ($(wildcard config.mak),)
$(error run ./configure.sh first. See ./configure.sh -h)
endif

include config.mak

O ?= .
OBJ_LIBOSDP := $(SRC_LIBOSDP:%.c=$(O)/%.o)
CCFLAGS += -Wall -Wextra -O3

ifeq ($(VERBOSE),)
MAKE := make -s
Q := @
else
Q :=
MAKE := make
endif

all: $(TARGETS)

$(O)/%.o: %.c
	@echo "  CC $<"
	@mkdir -p $(@D)
	$(Q)$(CC) -c $< $(CCFLAGS) $(CCFLAGS_EXTRA) -o $@

$(O)/libosdp.a: CCFLAGS_EXTRA=-Iutils/include -Iinclude -Isrc -I.
$(O)/libosdp.a: $(OBJ_LIBOSDP)
	@echo "  AR $@"
	$(Q)$(AR) qc $@ $^

## osdpctl

SRC_OSDPCTL := osdpctl/ini_parser.c osdpctl/config.c osdpctl/arg_parser.c
SRC_OSDPCTL += osdpctl/osdpctl.c osdpctl/cmd_start.c osdpctl/cmd_send.c
SRC_OSDPCTL += osdpctl/cmd_others.c utils/src/channel.c utils/src/procutils.c
SRC_OSDPCTL += utils/src/memory.c utils/src/hashmap.c utils/src/strutils.c
SRC_OSDPCTL += utils/src/utils.c utils/src/serial.c
OBJ_OSDPCTL := $(SRC_OSDPCTL:%.c=$(O)/%.o)

$(O)/osdpctl.elf: CCFLAGS_EXTRA=-Iosdpctl/include -Iutils/include -Iinclude
$(O)/osdpctl.elf: $(O)/libosdp.a $(OBJ_OSDPCTL)
	@echo "LINK $@"
	$(Q)$(CC) $(CCFLAGS) -o $@ $^ -L. -losdp

## Samples

$(O)/cp_app.elf: $(O)/libosdp.a
	@echo "LINK $@"
	$(Q)$(CC) $(CCFLAGS) samples/c/cp_app.c -o $@ -Iinclude -L. -losdp

$(O)/pd_app.elf: $(O)/libosdp.a
	@echo "LINK $@"
	$(Q)$(CC) $(CCFLAGS) samples/c/pd_app.c -o $@ -Iinclude -L. -losdp

## Clean

.PHONY: clean
clean:
	$(Q)rm -f $(O)/src/*.o $(O)/src/crypto/*.o $(OBJ_OSDPCTL) $(OBJ_TEST)
	$(Q)rm -f $(O)/*.a $(O)/*.elf

.PHONY: distclean
distclean: clean
	$(Q)rm -f config.mak $(O)/osdp_*.h
