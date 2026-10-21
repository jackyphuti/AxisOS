CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wno-unused-parameter -O2
CFLAGS += -DWLR_USE_UNSTABLE=1
CFLAGS += -I. $(shell pkg-config --cflags wlroots wayland-server xkbcommon libinput)
LIBS += $(shell pkg-config --libs wlroots wayland-server xkbcommon libinput)

WAYLAND_SCANNER ?= $(shell pkg-config --variable=wayland_scanner wayland-scanner 2>/dev/null || echo wayland-scanner)

TARGET = axis-compositor
GEN_SRC = xdg-shell-protocol.c wlr-layer-shell-unstable-v1-protocol.c
GEN_HDR = xdg-shell-protocol.h wlr-layer-shell-unstable-v1-protocol.h
OBJS = main.o $(GEN_SRC:.c=.o)

all: $(TARGET)

xdg-shell-protocol.h: protocols/xdg-shell.xml
	$(WAYLAND_SCANNER) server-header $< $@

xdg-shell-protocol.c: protocols/xdg-shell.xml xdg-shell-protocol.h
	$(WAYLAND_SCANNER) private-code $< $@

wlr-layer-shell-unstable-v1-protocol.h: protocols/wlr-layer-shell-unstable-v1.xml
	$(WAYLAND_SCANNER) server-header $< $@

wlr-layer-shell-unstable-v1-protocol.c: protocols/wlr-layer-shell-unstable-v1.xml wlr-layer-shell-unstable-v1-protocol.h
	$(WAYLAND_SCANNER) private-code $< $@

$(TARGET): $(GEN_HDR) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

main.o: main.c $(GEN_HDR)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET) *.o $(GEN_SRC) $(GEN_HDR)

.PHONY: all install clean
