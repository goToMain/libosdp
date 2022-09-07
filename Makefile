#
#  Copyright (c) 2021-2022 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

ifeq ($(wildcard config.make),)
$(error run ./configure.sh first. See ./configure.sh -h)
endif

include config.make

O ?= $(BUILD_DIR)
OBJ_LIBOSDP := $(SRC_LIBOSDP:%.c=$(O)/%.o)
OBJ_OSDPCTL := $(SRC_OSDPCTL:%.c=$(O)/%.o)
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
libosdp: $(O)/utils/libutils.a

.PHONY: pd_app
pd_app: $(O)/pd_app.elf

.PHONY: cp_app
cp_app: $(O)/cp_app.elf

.PHONY: libutils
libutils: $(O)/utils/libutils.a

.PHONY: osdpctl
osdpctl: libutils libosdp $(O)/osdpctl.elf

$(O)/%.o: %.c
	@echo "  CC $<"
	@mkdir -p $(@D)
	$(Q)$(CC) -c $< $(CCFLAGS) $(CCFLAGS_EXTRA) -o $@

$(O)/libosdp.a: CCFLAGS_EXTRA=-Iutils/include -Iinclude -Isrc -I$(O)
$(O)/libosdp.a: $(OBJ_LIBOSDP)
	@echo "  AR $(@F)"
	$(Q)$(AR) qc $@ $^

$(O)/utils/libutils.a:
	$(Q)make -C utils Q=$(Q) O=$(O)/utils CC=$(CC)

## osdpctl

$(O)/osdpctl.elf: CCFLAGS_EXTRA=-Iosdpctl/include -Iutils/include -Iinclude
$(O)/osdpctl.elf: $(OBJ_OSDPCTL)
	@echo "LINK $(@F)"
	$(Q)$(CC) $(CCFLAGS) -o $@ $^ -lpthread -L$(O) -losdp -L$(O)/utils -lutils

## Samples

$(O)/cp_app.elf: $(O)/libosdp.a
	@echo "LINK $(@F)"
	$(Q)$(CC) $(CCFLAGS) samples/c/cp_app.c -o $@ -Iinclude -L$(O) -losdp

$(O)/pd_app.elf: $(O)/libosdp.a
	@echo "LINK $(@F)"
	$(Q)$(CC) $(CCFLAGS) samples/c/pd_app.c -o $@ -Iinclude -L$(O) -losdp

## Tests

.PHONY: check
check: CCFLAGS_EXTRA=-DUNIT_TESTING -DCONFIG_OSDP_FILE -Iutils/include -Iinclude -Isrc -I$(O)
check: clean $(OBJ_TEST)
	@echo "LINK $@"
	$(Q)$(CC) $(CCFLAGS) $(OBJ_TEST) -o /tmp/check
	$(Q)/tmp/check
	$(Q)rm -rf /tmp/check && make clean

## Clean

.PHONY: clean
clean:
	$(Q)rm -f $(O)/src/*.o $(O)/src/crypto/*.o $(OBJ_OSDPCTL) $(OBJ_TEST)
	$(Q)rm -f $(O)/*.a $(O)/*.elf
	$(Q)make -C utils Q=$(Q) O=$(O)/utils clean

.PHONY: distclean
distclean: clean
	$(Q)rm -rf config.make $(O)
