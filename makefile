# Root Makefile

CC ?= gcc

# Adjust to your system
PICO_SDK_INC := /opt/picoscope/include
PICO_SDK_LIB := /opt/picoscope/lib

CPPFLAGS := -I./simple_pico -I$(PICO_SDK_INC)
CFLAGS   := -O2 -g -Wall -Wextra -Wpedantic -std=c11

LDFLAGS  := -L$(PICO_SDK_LIB)
LDLIBS   := -lps3000a -lpthread -lm

DEMO_SRC := demo.c
DEMO_BIN := demo

LIB_DIR := simple_pico
LIB_A   := $(LIB_DIR)/build/libsimple_pico.a

# Discover sources and headers
LIB_SRCS := $(wildcard simple_pico/*.c)
LIB_HDRS := $(wildcard simple_pico/*.h)

.PHONY: all clean

all: $(DEMO_BIN)

$(LIB_A): $(LIB_HDRS) $(LIB_SRCS)
	$(MAKE) -C $(LIB_DIR) \
	  CPPFLAGS_EXTRA="$(CPPFLAGS)"

$(DEMO_BIN): $(DEMO_SRC) $(LIB_A)
	$(CC) $(CPPFLAGS) $(CFLAGS) \
	  $(DEMO_SRC) $(LIB_A) \
	  $(LDFLAGS) $(LDLIBS) \
	  -o $@

clean:
	$(MAKE) -C $(LIB_DIR) clean
	rm -f $(DEMO_BIN)
