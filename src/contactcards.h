/* Copyright (c) 2013-2016 Florian L. <dev@der-flo.net>
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

/**

    Some words on flags:

    The first 8 fields are for general orientation:

     +----------------
     | +--------------
     | | +------------
     | | | +----------
     | | | | +--------
     | | | | | +------ graphical stuff 
     | | | | | | +---- DB stuff
     | | | | | | | +-- general stuff
     v v v v v v v v
    +-+-+-+-+-+-+-+-+
    | | | | | | | | |
    +-+-+-+-+-+-+-+-+
   8                 0

    The second 8 bits on combination with "general stuff"
     +----------------
     | +--------------
     | | +------------
     | | | +----------
     | | | | +--------
     | | | | | +------ query 
     | | | | | | +---- debug
     | | | | | | | +-- verbose
     v v v v v v v v
    +-+-+-+-+-+-+-+-+
    | | | | | | | | |
    +-+-+-+-+-+-+-+-+
  16                 8

    The second 8 bits on combination with "DB stuff"
     +----------------
     | +--------------
     | | +------------
     | | | +---------- no passwd
     | | | | +-------- local
     | | | | | +------ favorit 
     | | | | | | +---- one way
     | | | | | | | +-- tmp
     v v v v v v v v
    +-+-+-+-+-+-+-+-+
    | | | | | | | | |
    +-+-+-+-+-+-+-+-+
  16                 8


    The second 8 bits on combination with "graphical stuff"
     +----------------
     | +-------------- no local address book
     | | +------------ separator
     | | | +---------- google maps
     | | | | +-------- open street map
     | | | | | +------ family name only 
     | | | | | | +---- given name first
     | | | | | | | +-- family name first
     v v v v v v v v
    +-+-+-+-+-+-+-+-+
    | | | | | | | | |
    +-+-+-+-+-+-+-+-+
  16                 8

 */

#define	CONTACTCARDS_VERBOSE		0x101
#define	CONTACTCARDS_DEBUG			0x201
#define	CONTACTCARDS_QUERY			0x401

#define	CONTACTCARDS_TMP			0x0102
#define	CONTACTCARDS_ONE_WAY_SYNC	0x0202
#define	CONTACTCARDS_FAVORIT		0x0402
#define	CONTACTCARDS_LOCAL			0x0802
#define	CONTACTCARDS_NO_PASSWD		0x1002


#define	DISPLAY_STYLE_MASK					0x0704
#define	FAMILYNAME_FIRST			0x0104
#define	GIVENNAME_FIRST				0x0204
#define	FAMILYNAME_ONLY				0x0404
#define	USE_MAP_MASK						0x1804
#define	USE_OSM						0x0804
#define	USE_GOOGLE					0x1004
#define	USE_SEPARATOR				0x2004		/*	Unused so far	*/
#define	CONTACTCARDS_NO_LOCAL		0x4004
#define CONTACTCARDS_HIDE_CAL       0x8004


#define	__PRINTFUNC__	g_log("ContactCards", G_LOG_LEVEL_DEBUG, "[%s]\n", __func__);

#endif	/*	ContactCards_H		*/
