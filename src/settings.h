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

#ifndef	settings_H
#define	settings_H

/**
 * struct ContactCards_app -struct for settings
 * It's a struct to be flexible for the future.
 */
typedef struct ContactCards_app {
	char		*configdir;
	gboolean	debug;
	gboolean	verbose;
} ContactCards_app_t;

extern void debugCC(gchar const *format, ...);
extern void verboseCC(gchar const *format, ...);
extern void checkAndSetConfig(ContactCards_app_t *app);
extern void config_load(ContactCards_app_t *app);
extern ContactCards_app_t *parseCmdLine(int *argc, char **argv[]);

#endif	/*	settings_H		*/
