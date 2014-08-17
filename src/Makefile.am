AM_CFLAGS = -Wall -Wno-pointer-sign -std=c99 -ggdb -lpthread -O3
AM_CFLAGS += -DLOCALEDIR=\"$(localedir)\"

bin_PROGRAMS = contactcards
contactcards_SOURCES = 	contactcards.c \
						cards.c \
						gui.c \
						gui-dialog.c \
						neon_rx.c \
						neon_tx.c \
						settings.c \
						sqlite_frontend.c \
						sqlite_backend.c

AM_CFLAGS += $(GTK3_CFLAGS) $(NEON_CFLAGS) $(SQLITE3_CFLAGS)
AM_LDFLAGS = $(GTK3_LIBS) $(NEON_LIBS) $(SQLITE3_LIBS)
