# AxisOS System Update Engine Makefile
CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic -D_GNU_SOURCE -std=c11
TARGET = axis
DESTDIR ?=
PREFIX ?= /usr

all: $(TARGET)

$(TARGET): axis_update.c axis_update.h
	$(CC) $(CFLAGS) axis_update.c -o $(TARGET)
	ln -sf $(TARGET) axis-update

clean:
	rm -f $(TARGET) axis-update *.o

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	ln -sf $(TARGET) $(DESTDIR)$(PREFIX)/bin/axis-update
	install -d $(DESTDIR)/etc/systemd/system
	install -m 644 axis-mark-good.service $(DESTDIR)/etc/systemd/system/axis-mark-good.service

.PHONY: all clean install
