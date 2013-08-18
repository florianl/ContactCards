/*
 *	main.c
 */

#ifndef ContactCards_H
#include "ContactCards.h"
#endif

int main(int argc, char **argv){
	printfunc(__func__);

	int					ret;
	sqlite3				*db_handler;

	ret = sqlite3_open_v2(DATABASE, &db_handler, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	sqlite3_extended_result_codes(db_handler, TRUE);

	if (ret != SQLITE_OK) {
		printf("Error occured connecting to the database. Errorcode: %d\n", ret);
		return ret;
	}

	dbCreate(db_handler);

	guiInit(db_handler);
	g_mutex_init(&mutex);

	newOAuthEntity(db_handler, "google.com", "741969998490.apps.googleusercontent.com", "71adV1QbUKszvBV_xXliTD34", "https://www.googleapis.com/.well-known/carddav", "https://www.googleapis.com/auth/carddav", "https://accounts.google.com/o/oauth2/auth", "https://accounts.google.com/o/oauth2/token", "code", "urn:ietf:wg:oauth:2.0:oob", "authorization_code");

	#ifdef ContactCards_DEBUG
	showServer(db_handler);
	showAddressbooks(db_handler);
	showContacts(db_handler);
	#endif

	guiRun(db_handler);

	dbClose(db_handler);

	return 0;
}
