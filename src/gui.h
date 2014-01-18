/*
 *	gui.h
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

#ifndef	CC_trans
#define CC_trans
typedef struct ContactCards_trans {
	sqlite3				*db;
	void				*element;
	void				*element2;
} ContactCards_trans_t;
#endif	/*	CC_trans			*/

enum {
	TEXT_COLUMN = 0,
	ID_COLUMN,
	N_COLUMNS
};

extern void guiRun(sqlite3 *ptr);
extern void guiInit(sqlite3 *ptr);

extern void listAppend(GtkWidget *list, gchar *text, guint id);
extern void comboAppend(GtkListStore *store, gchar *text, guint id);
extern void listFlush(GtkWidget *list);
extern void comboFlush(GtkListStore *store);
extern void dialogRequestGrant(sqlite3 *ptr, int serverID, int entity, char *newuser);

typedef struct ContactCards_pref {
	GtkEntryBuffer		*descBuf;
	GtkEntryBuffer		*urlBuf;
	GtkEntryBuffer		*userBuf;
	GtkEntryBuffer		*passwdBuf;
	GtkEntryBuffer		*issuedBuf;
	GtkEntryBuffer		*issuerBuf;
	GtkWidget			*btnDel;
	GtkWidget			*btnSave;
	GtkWidget			*btnExportCert;
	GtkWidget			*srvPrefList;
	GtkWidget			*certSel;
	int					srvID;
} ContactCards_pref_t;

typedef struct ContactCards_add {
	sqlite3				*db;
	GtkWidget			*grid;
	GSList				*list;
	int					addrBookID;
	GtkEntryBuffer		*fnBuff;
	GtkEntryBuffer		*lnBuff;
} ContactCards_add_t;

#define	CONTACT_ADD_WINDOW		1

typedef struct ContactCards_item {
	int			itemID;
	void		*element;
} ContactCards_item_t;

#endif	/*	gui_H	*/
