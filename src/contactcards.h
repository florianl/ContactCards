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

#ifndef ContactCards_H
#define ContactCards_H

#ifndef std_H
#define std_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib/gi18n.h>
#include <glib-unix.h>
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
GtkListStore			*comboList;
GMutex 					mutex;
GMutex 					aBookTreeMutex;
GMutex					contactsTreeMutex;
sqlite3_mutex			*dbMutex;

#define	CONTACTCARDS_TMP			0x1
#define	CONTACTCARDS_ONE_WAY_SYNC	0x2
#define	CONTACTCARDS_FAVORIT		0x4
#define	CONTACTCARDS_LOCAL			0x8
#define	CONTACTCARDS_QUERY			0x10

#define	__PRINTFUNC__	g_log("ContactCards", G_LOG_LEVEL_DEBUG, "[%s]\n", __func__);

#endif	/*	ContactCards_H		*/
