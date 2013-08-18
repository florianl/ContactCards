# BASICS
CC=gcc
CFLAGS=-Wall -Wno-pointer-sign -std=c99 -ggdb -lpthread -O3
PKGCONFIG=pkg-config

# GRAPHIC STUFF
LIBGTK = $(shell $(PKGCONFIG) --libs gtk+-3.0 glib-2.0)
GLIB2CFLAGS = $(shell $(PKGCONFIG) --cflags glib-2.0)
GTKCFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-3.0)

# NEON STUFF
LIBNEON = $(shell neon-config --libs 2> /dev/null)
ifneq ($(strip $(LIBNEON)),)
	NEONFLAGS = $(shell neon-config --cflags)
endif

# SQLITE3 STUFF
LIBSQLITE3 = $(shell $(PKGCONFIG) --libs sqlite3 2> /dev/null)
ifneq ($(strip $(LIBSQLITE3)),)
	SQLITE3 = -DSQLITE3 $(shell $(PKGCONFIG) --cflags sqlite3)
endif

LIBS = $(LIBGTK) $(LIBSQLITE3) $(LIBNEON)

EXTRA_FLAGS = $(SQLITE3) $(GTKCFLAGS) $(NEONFLAGS) $(GLIB2CFLAGS)

BIN = ContactCards

OBJS = main.o gui.o sqlite_frontend.o sqlite_backend.o neon_rx.o neon_tx.o

all: $(BIN)

$(BIN):$(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

%.o: %.c
	@echo '    CC' $<
	@$(CC) $(CFLAGS) $(EXTRA_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf $(BIN) $(OBJS)
