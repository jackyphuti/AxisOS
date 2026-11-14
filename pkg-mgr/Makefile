CC ?= gcc
CFLAGS ?= -std=gnu11 -Wall -Wextra -O2
PKG_CONFIG ?= pkg-config

DEPS = libcurl json-c libarchive openssl
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(DEPS))
LIBS += $(shell $(PKG_CONFIG) --libs $(DEPS))

BIN_DIR = bin
TARGETS = $(BIN_DIR)/axis $(BIN_DIR)/axis_repo_builder

all: $(TARGETS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/axis: src/axis.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/axis.c $(LIBS)

$(BIN_DIR)/axis_repo_builder: src/axis_repo_builder.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/axis_repo_builder.c $(LIBS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
