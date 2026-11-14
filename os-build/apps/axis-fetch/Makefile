CC ?= gcc
CFLAGS ?= -std=gnu11 -Wall -Wextra -O2
BIN = axis-fetch

all: $(BIN)

$(BIN): main.c
	$(CC) $(CFLAGS) -o $@ $<

install: $(BIN)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(BIN) $(DESTDIR)/usr/local/bin/

clean:
	rm -f $(BIN)

.PHONY: all install clean
