AM_CFLAGS = -Wall -Wno-pointer-sign -std=c99 -ggdb -lpthread -O3
AM_CFLAGS += -DLOCALEDIR=\"$(localedir)\"

bin_PROGRAMS = ContactCards
ContactCards_SOURCES = 	ContactCards.c \
						cards.c \
						gui.c \
						neon_rx.c \
						neon_tx.c \
						sqlite_frontend.c \
						sqlite_backend.c

ContactCards_CPPFLAGS = $(GTK3_CFLAGS) $(NEON_CFLAGS) $(SQLITE3_CFLAGS)
ContactCards_LDADD = $(GTK3_LIBS) $(NEON_LIBS) $(SQLITE3_LIBS)
