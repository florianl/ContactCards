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

#include "contactcards.h"

/**
 * syncTimer - Callback for the sync timer
 */
gboolean syncTimer(gpointer data){
	__PRINTFUNC__;

	syncServer(NULL, NULL);

	return TRUE;
}

/**
 * main - do I need to say more?
 */
int main(int argc, char **argv){
	__PRINTFUNC__;

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
	config_load(app);

	db = g_build_filename(app->configdir, "ContactCards.sql", NULL);

	ret = sqlite3_open_v2(db, &db_handler, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
	sqlite3_extended_result_codes(db_handler, TRUE);

	if (ret != SQLITE_OK) {
		verboseCC("[%s] Error occured while connecting to the database\n", __func__);
		verboseCC("[%s] Errorcode: %d\n", __func__, ret);
		return ret;
	}

	dbMutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
	if(dbMutex == NULL){
		verboseCC("[%s] mutex for the database could not be allocated\n", __func__);
		return -1;
	}

	dbCreate(db_handler);
	if(dbCheck(db_handler)){
		verboseCC("[%s] Error while checking database\n", __func__);
		return -1;
	}
	appBase.db = db_handler;
	appBase.flags |= app->flag;

	g_mutex_init(&mutex);
	g_mutex_init(&aBookTreeMutex);
	g_mutex_init(&contactsTreeMutex);

	if((app->flag & CONTACTCARDS_QUERY) == CONTACTCARDS_QUERY){
		queryAddressbooks(app->query);
		g_free(app->query);
	} else {
		guiInit();

		newOAuthEntity(db_handler, "google.com", "741969998490.apps.googleusercontent.com", "71adV1QbUKszvBV_xXliTD34", "https://www.googleapis.com/.well-known/carddav", "https://www.googleapis.com/auth/carddav", "https://accounts.google.com/o/oauth2/auth", "https://accounts.google.com/o/oauth2/token", "code", "urn:ietf:wg:oauth:2.0:oob", "authorization_code");

		if((app->flag & CONTACTCARDS_DEBUG) == CONTACTCARDS_DEBUG){
			showServer(db_handler);
			showAddressbooks(db_handler);
			showContacts(db_handler);
		}

		g_timeout_add_seconds (appBase.syncIntervall, syncTimer, NULL);
		guiRun(db_handler);
	}
	dbClose(db_handler);

	saveSettings(app->configdir);

	g_free(db);
	sqlite3_mutex_free(dbMutex);

	if(app->configdir)
		g_free(app->configdir);
	g_free(app);

	g_mutex_clear(&mutex);
	g_mutex_clear(&aBookTreeMutex);
	g_mutex_clear(&contactsTreeMutex);

	return 0;
}
