/*
 * ContactCards.h
 */

#ifndef ContactCards_H
#define ContactCards_H

#ifndef std_H
#define std_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>
#define _(string)		gettext(string)
#endif	/*	std_H		*/

#ifndef	gui_H
#include "gui.h"
#endif	/*	gui_H		*/

#ifndef sqlite_H
#include "sqlite_db.h"
#endif	/*	sqlite_H	*/

#ifndef neon_H
#include "neon.h"
#endif	/*	neon_H		*/

/*	GLOBALES		*/
GtkWidget				*addressbookList, *contactList;
GtkListStore			*comboList;
GMutex 					mutex;
GtkWidget				*mainWindow;
int						selectedSrv;

// #define ContactCards_DEBUG
#ifdef ContactCards_DEBUG
#define printfunc(X)	printf("[%s]\n", X)
#else
#define printfunc(X)	do { } while (0)
#endif

#define VERSION			"0.02-devel"
#define DATABASE		"ContactCards.sql"

typedef struct ContactCards_trans {
	sqlite3				*db;
	void				*element;
	void				*element2;
} ContactCards_trans_t;

#endif	/*	ContactCards_H		*/
