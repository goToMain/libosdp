ifeq ($(wildcard config.mak),)
$(error run ./configure first. See ./configure -h)
endif

include config.mak

O ?= .
OBJ := $(SRC:%.c=$(O)/%.o)
CCFLAGS += -Wall -Wextra -O3 -Iinclude -Isrc -I. -Iutils/include

ifeq ($(VERBOSE),)
MAKE := make -s
Q := @
else
Q :=
MAKE := make
endif

all: $(O)/libosdp.a samples

$(O)/libosdp.a: $(OBJ)
	@echo "  AR $@"
	$(Q)$(AR) qc $@ $^

$(O)/%.o: %.c
	@echo "  CC $<"
	@mkdir -p $(@D)
	$(Q)$(CC) -c $< $(CCFLAGS) -o $@

.PHONY: samples
samples: ${OSDP_SAMPLES}

cp_app.out:
	@echo "LINK $@"
	$(Q)$(CC) $(CCFLAGS) samples/c/cp_app.c -o cp_app.out -Iinclude -L. -losdp

pd_app.out:
	@echo "LINK $@"
	$(Q)$(CC) $(CCFLAGS) samples/c/pd_app.c -o pd_app.out -Iinclude -L. -losdp

.PHONY: clean
clean:
	$(Q)rm -f $(OBJ) $(O)/src/*.o $(O)/libosdp.a cp_app.out pd_app.out

.PHONY: distclean
distclean: clean
	$(Q)rm -f config.mak osdp_config.h osdp_export.h
