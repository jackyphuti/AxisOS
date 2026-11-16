CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wno-unused-parameter -O2
WLR_PKG ?= $(shell for pkg in wlroots-0.18 wlroots-0.17 wlroots; do if pkg-config --exists $$pkg 2>/dev/null; then echo $$pkg; break; fi; done)

ifeq ($(WLR_PKG),)
$(error "No compatible wlroots package (wlroots-0.18, wlroots-0.17, or wlroots) found via pkg-config")
endif

CFLAGS += -DWLR_USE_UNSTABLE=1
CFLAGS += -I. $(shell pkg-config --cflags $(WLR_PKG) wayland-server xkbcommon libinput libdrm)
LIBS += $(shell pkg-config --libs $(WLR_PKG) wayland-server xkbcommon libinput libdrm)

WAYLAND_SCANNER ?= $(shell pkg-config --variable=wayland_scanner wayland-scanner 2>/dev/null || echo wayland-scanner)

TARGET = axis-compositor

PROTO_XMLS = $(wildcard protocols/*.xml)
PROTO_BASENAMES = $(notdir $(basename $(PROTO_XMLS)))
GEN_HDR = $(addsuffix -protocol.h, $(PROTO_BASENAMES))
GEN_SRC = $(addsuffix -protocol.c, $(PROTO_BASENAMES))

OBJS = main.o $(GEN_SRC:.c=.o)

all: $(TARGET)

%-protocol.h: protocols/%.xml
	$(WAYLAND_SCANNER) server-header $< $@

%-protocol.c: protocols/%.xml %-protocol.h
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
