CC ?= gcc
CFLAGS ?= -std=gnu11 -Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0)
LIBS ?= $(shell pkg-config --libs gtk+-3.0) -lm
BIN = axis-color-picker

all: $(BIN)

$(BIN): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

install: $(BIN)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(BIN) $(DESTDIR)/usr/local/bin/
	install -d $(DESTDIR)/usr/share/applications
	install -m 644 axis-color-picker.desktop $(DESTDIR)/usr/share/applications/

clean:
	rm -f $(BIN)

.PHONY: all install clean
