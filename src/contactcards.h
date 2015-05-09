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
#include <glib.h>
#include <glib/gstdio.h>
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

#define	CONTACTCARDS_TMP			0x1		/*	For DB only	*/
#define	CONTACTCARDS_ONE_WAY_SYNC	0x2
#define	CONTACTCARDS_FAVORIT		0x4		/*	For DB only	*/
#define	CONTACTCARDS_LOCAL			0x8		/*	For DB only	*/

#define	CONTACTCARDS_VERBOSE		0x10
#define	CONTACTCARDS_DEBUG			0x20

#define	CONTACTCARDS_NO_LOCAL		0x40
#define	CONTACTCARDS_QUERY			0x80

#define	DISPLAY_STYLE_MASK			0x700
#define	FAMILYNAME_FIRST			0x100
#define	GIVENNAME_FIRST				0x200
#define	FAMILYNAME_ONLY				0x400



#define	USE_MAP_MASK		0x3000
#define	USE_OSM				0x1000
#define	USE_GOOGLE			0x2000

#define		USE_SEPARATOR		0x08	/*	Unused so far	*/

#define	__PRINTFUNC__	g_log("ContactCards", G_LOG_LEVEL_DEBUG, "[%s]\n", __func__);

#endif	/*	ContactCards_H		*/
