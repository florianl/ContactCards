/* Copyright (c) 2013-2015 Florian L. <dev@der-flo.net>
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

#include <gtk/gtk.h>

#ifndef sqlite_H
#include "sqlite_db.h"
#endif	/*	sqlite_H	*/

typedef struct ContactCardsGuiBase{
	sqlite3		*db;
	GtkWidget	*window;
	GtkWidget	*statusbar;
	GtkWidget	*syncMenu;
	GtkWidget	*addressbookList;
	GtkWidget	*contactList;
	GtkWidget	*contactView;
	GtkWidget	*cal;
	GSList		*callist;
	int			flags;
	int			syncIntervall;
}ContactCardsGuiBase;

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
	FN_COLUMN = 0,
	FIRST_COLUMN,
	LAST_COLUMN,
	SELECTION_COLUMN,
	SEP_COLUMN,
	TOTAL_COLUMNS
};

enum {
	TEXT_COLUMN,
	ID_COLUMN
};

extern void guiRun(sqlite3 *ptr);
extern void guiInit(void);

extern void listAppend(GtkWidget *list, gchar *text, guint id);
extern gchar *birthdayTooltip(GtkCalendar *cal, guint year, guint month, guint day, gpointer trans);
extern void calendarUpdate(int type, int id);
extern void comboAppend(GtkListStore *store, gchar *text, guint id);
extern void dialogKeyHandler(GtkDialog *widget, GdkEventKey *event, gpointer data);
extern void feedbackDialog(int type, char *msg);
extern void listInit(GtkWidget *list);
extern void listFlush(GtkWidget *list);
extern void comboFlush(GtkListStore *store);
extern void *syncOneServer(void *trans);
extern void addressbookTreeUpdate(void);
extern void contactsTreeUpdate(int type, int id);
extern void contactsTreeAppend(char *card, int id);
//extern void contactsTreeSetSeperators(void);
extern GtkWidget *addressbookTreeCreate(void);
extern void cbAddrBookExportBirthdays(GtkMenuItem *menuitem, gpointer data);
extern void cbSrvExportBirthdays(GtkMenuItem *menuitem, gpointer data);
extern void syncServer(GtkWidget *widget, gpointer trans);
extern void viewCleaner(GtkWidget *widget);

/*		gui-dialog.c	*/
extern void newDialog(GtkWidget *do_widget, gpointer trans);
extern void requestPasswd(credits_t *key, int serverID);
extern void prefWindow(GtkWidget *widget, gpointer trans);
extern void syncMenuUpdate();
extern void syncMenuSel(GtkWidget *widget, gpointer trans);
extern void birthdayDialog(GtkWidget *widget, gpointer trans);

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
	GtkWidget			*syncSel;
	GtkWidget			*listbox;
	GtkWidget			*colorChooser;
	GtkWidget			*sPasswd;
	GSList				*aBooks;
	int					srvID;
#ifdef _USE_DANE
	GtkWidget			*dane;
#endif	/*	_USE_DANE	*/
} ContactCards_pref_t;


/**
 * struct ContactCards_genPref	- structure for handling the general settings
 */
typedef struct ContactCards_genPref {
	GtkWidget			*sort;
	GtkWidget			*map;
	GtkWidget			*interval;
	GtkWidget			*locals;
} ContactCards_genPref_t;

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

/**
 * ContactCards_cal_item	- struct keep the tooltip for the calendar simple
 */
typedef struct ContactCards_cal_item {
	int			day;
	char		*txt;
} ContactCards_cal_item_t;

#endif	/*	gui_H	*/
