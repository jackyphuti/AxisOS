CC ?= gcc
CFLAGS ?= -std=gnu11 -Wall -Wextra -O2
PKG_CONFIG ?= pkg-config

DEPS = gtk+-3.0 gtk-layer-shell-0 gio-2.0 pam libudev
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(DEPS))
LIBS += $(shell $(PKG_CONFIG) --libs $(DEPS)) -lm

TARGET = axisos-shell
SRCS = main.c system_controller.c auth_manager.c ui_shell.c
OBJS = $(SRCS:.c=.o)

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

all: $(TARGET) LiquidNodesOS

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

LiquidNodesOS: $(TARGET)
	ln -sf $(TARGET) LiquidNodesOS

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) LiquidNodesOS

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	ln -sf $(TARGET) $(DESTDIR)$(BINDIR)/LiquidNodesOS

.PHONY: all clean install
