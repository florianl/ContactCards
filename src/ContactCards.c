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

#include "ContactCards.h"

/**
 * main - do I need to say more?
 */
int main(int argc, char **argv){
	printfunc(__func__);

	int					ret;
	sqlite3				*db_handler;
	ContactCards_app_t	*app;
	char				*db;

	/* Set up internationalization */
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	app = parseCmdLine(&argc, &argv);

	checkAndSetConfig(app);

	db = g_build_filename(app->configdir, "ContactCards.sql", NULL);

	ret = sqlite3_open_v2(db, &db_handler, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	sqlite3_extended_result_codes(db_handler, TRUE);

	if (ret != SQLITE_OK) {
		printf("Error occured connecting to the database. Errorcode: %d\n", ret);
		return ret;
	}

	dbCreate(db_handler);

	guiInit(db_handler);
	g_mutex_init(&mutex);


	newOAuthEntity(db_handler, "google.com", "741969998490.apps.googleusercontent.com", "71adV1QbUKszvBV_xXliTD34", "https://www.googleapis.com/.well-known/carddav", "https://www.googleapis.com/auth/carddav", "https://accounts.google.com/o/oauth2/auth", "https://accounts.google.com/o/oauth2/token", "code", "urn:ietf:wg:oauth:2.0:oob", "authorization_code");

	if(app->debug){
		showServer(db_handler);
		showAddressbooks(db_handler);
		showContacts(db_handler);
	}

	guiRun(db_handler);

	dbClose(db_handler);

	g_free(app->configdir);
	g_free(db);

	return 0;
}
