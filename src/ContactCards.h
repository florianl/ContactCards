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

// #define ContactCards_FUNC
#ifdef ContactCards_FUNC
#define printfunc(X)	printf("[%s]\n", X)
#else
#define printfunc(X)	do { } while (0)
#endif

// #define ContactCards_DEBUG
#ifdef ContactCards_DEBUG
#define G_LOG_PROTO		(8)
#define dbgCC(...)	g_log(NULL, G_LOG_PROTO, __VA_ARGS__)
#else
#define dbgCC(...)	do { } while (0)
#endif  /* ContactCards_DEBUG */

#endif	/*	ContactCards_H		*/
