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
#include <glib/gi18n.h>
#endif	/*	std_H		*/

#ifndef	gui_H
#include "gui.h"
#endif	/*	gui_H		*/

#ifndef sqlite_H
#include "sqlite_db.h"
#endif	/*	sqlite_H	*/

#ifndef cards_H
#include "cards.h"
#endif	/*	cards_H		*/

#ifndef neon_H
#include "neon.h"
#endif	/*	neon_H		*/

#ifndef settings_H
#include "settings.h"
#endif	/*	settings_H	*/

/*	GLOBALS		*/
GtkWidget				*addressbookList, *contactList;
GtkListStore			*comboList;
GMutex 					mutex;
GtkWidget				*mainWindow;
int						selectedSrv;

#define	printfunc(X)	g_log("ContactCards", G_LOG_LEVEL_DEBUG, "[%s]\n", X);

#endif	/*	ContactCards_H		*/
