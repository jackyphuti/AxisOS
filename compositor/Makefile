CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wno-unused-parameter -O2
CFLAGS += -DWLR_USE_UNSTABLE=1
CFLAGS += -I. $(shell pkg-config --cflags wlroots wayland-server xkbcommon)
LIBS += $(shell pkg-config --libs wlroots wayland-server xkbcommon)

TARGET = axis-compositor
OBJS = main.o xdg-shell-protocol.o wlr-layer-shell-unstable-v1-protocol.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET) *.o

.PHONY: all install clean
