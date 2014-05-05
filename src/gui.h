/* Copyright (c) 2013-2014 Florian L. <dev@der-flo.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef	gui_H
#define gui_H

#ifndef	GTK_LIB_H
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#define GTK_LIB_H
#endif	/*	GTK_LIB_H	*/

#ifndef SQLITE3_LIB_H
#define SQLITE3_LIB_H
#include <sqlite3.h>
#endif	/*	SQLITE3_LIB_H	*/

typedef struct ContactCardsGuiBase{
	sqlite3		*db;
	GtkWidget	*window;
	GtkWidget	*statusbar;
	GtkWidget	*syncMenu;
	GtkWidget	*addressbookList;
	GtkWidget	*contactList;
	GtkWidget	*contactView;
}
ContactCardsGuiBase;

ContactCardsGuiBase appBase;

/*	Adress Book Tree	*/
enum {
	DESC_COL = 0,
	ID_COL,
	TYP_COL,
	TOTAL_COLS
};

/*	Contacts Tree	*/
enum {
	TEXT_COLUMN,
	ID_COLUMN
};

extern void guiRun(sqlite3 *ptr);
extern void guiInit(void);

extern void listAppend(GtkWidget *list, gchar *text, guint id);
extern void comboAppend(GtkListStore *store, gchar *text, guint id);
extern void dialogKeyHandler(GtkDialog *widget, GdkEventKey *event, gpointer data);
extern void feedbackDialog(int type, char *msg);
extern void listInit(GtkWidget *list);
extern void listFlush(GtkWidget *list);
extern void comboFlush(GtkListStore *store);
extern void *syncOneServer(void *trans);
extern void addressbookTreeUpdate(void);

/*		gui-dialog.c	*/
extern void newDialog(GtkWidget *do_widget, gpointer trans);
extern void prefWindow(GtkWidget *widget, gpointer trans);
extern void syncMenuUpdate();

/**
 * struct ContactCards_pref	- structure for handling the preference dialog
 */
typedef struct ContactCards_pref {
	GtkWidget			*prefFrame;
	GtkEntryBuffer		*descBuf;
	GtkEntryBuffer		*urlBuf;
	GtkEntryBuffer		*userBuf;
	GtkEntryBuffer		*passwdBuf;
	GtkEntryBuffer		*issuedBuf;
	GtkEntryBuffer		*issuerBuf;
	GtkWidget			*srvPrefList;
	GtkWidget			*certSel;
	GtkWidget			*listbox;
	GSList				*aBooks;
	int					srvID;
} ContactCards_pref_t;

/**
 * struct ContactCards_aBooks	- structure for handling address books in the preferences dialog
 */
typedef struct ContactCards_aBooks {
	int					aBookID;
	GtkWidget			*check;
} ContactCards_aBooks_t;

/**
 * struct ContactCards_pix	- structure for handling a picture of a vCard
 */
typedef struct ContactCards_pix {
	guchar				*pixel;
	int					size;
} ContactCards_pix_t;

/**
 * struct ContactCards_add	- structure for creating a new vCard
 */
typedef struct ContactCards_add {
	sqlite3				*db;
	GtkWidget			*grid;
	GSList				*list;
	int					editID;
	int					aID;
} ContactCards_add_t;

/**
 * struct ContactCards_new_Value	- structure for adding a new value to a vCard
 */
typedef struct ContactCards_new_Value {
	GtkWidget			*grid;
	GSList				*list;
	int					type;
} ContactCards_new_Value_t;

/**
 * struct ContactCards_add	- helping structure for creating a new vCard
 */
typedef struct ContactCards_item {
	int			itemID;
	void		*element;
} ContactCards_item_t;

#endif	/*	gui_H	*/
