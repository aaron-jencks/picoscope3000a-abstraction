# simple_pico/Makefile
# Builds libsimple_pico.a from all .c files in this folder (recursively optional).

# ---- user-tweakable ----
LIB_NAME     := simple_pico
BUILD_DIR    := build
INCLUDE_DIRS := .              # add include dirs here (space-separated)
SRC_DIRS     := .              # add src dirs here (space-separated)

# Extra flags can be injected by the parent via:
#   make -C simple_pico CFLAGS_EXTRA="..." LDFLAGS_EXTRA="..."
CFLAGS_EXTRA  ?=
CPPFLAGS_EXTRA?=
LDFLAGS_EXTRA ?=
LDLIBS_EXTRA  ?=

# ---- toolchain ----
CC      ?= gcc
AR      ?= ar
RANLIB  ?= ranlib

# ---- flags ----
CPPFLAGS := $(addprefix -I,$(INCLUDE_DIRS)) $(CPPFLAGS_EXTRA)
CFLAGS   := -O2 -g -Wall -Wextra -Wpedantic $(CFLAGS_EXTRA)
LDFLAGS  := $(LDFLAGS_EXTRA)
LDLIBS   := $(LDLIBS_EXTRA)

# ---- sources/objects ----
# Find all .c in SRC_DIRS (non-recursive per dir); if you want recursive, see note below.
SRCS := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.c))
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

LIB  := $(BUILD_DIR)/lib$(LIB_NAME).a

.PHONY: all clean print-vars

all: $(LIB)

# Ensure build subdirs exist
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile rule (keeps subpaths under build/)
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Archive
$(LIB): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

clean:
	rm -rf $(BUILD_DIR)

# Helpful for parent Makefile debugging
print-vars:
	@echo "LIB=$(LIB)"
	@echo "CPPFLAGS=$(CPPFLAGS)"
	@echo "CFLAGS=$(CFLAGS)"
