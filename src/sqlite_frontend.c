/*
 * sqlite_frontend.c
 */

#ifndef ContactCards_H
#include "ContactCards.h"
#endif

void fillList(sqlite3 *ptr, int type, int from, GtkWidget *list){
	printfunc(__func__);

	sqlite3_stmt	 	*vm;
	char 				*sql_query = NULL;
	int					ret;

	listFlush(list);

	switch(type){
		case 1:		// addressbooklist
			if(from == 0){
				sql_query = sqlite3_mprintf("SELECT addressbookID, displayname FROM addressbooks;");
			} else {
				sql_query = sqlite3_mprintf("SELECT addressbookID, displayname FROM addressbooks WHERE cardServer = '%d';", from);
			}
			break;
		case 2:		// contactlist
			sql_query = sqlite3_mprintf("SELECT contactID, displayname FROM contacts WHERE addressbookID = '%d';", from);
			break;
		case 3:		// serverlist
			sql_query = sqlite3_mprintf("SELECT serverID, desc FROM cardServer;");
			break;
		default:
			printf("[%s] can't handle this type: %d\n", __func__, type);
			return;
	}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		printf("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return;
	}

	while(sqlite3_step(vm) != SQLITE_DONE){
		listAppend(list, (gchar *) sqlite3_column_text(vm, 1), (guint) sqlite3_column_int(vm, 0));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
}

void fillCombo(sqlite3 *ptr, GtkListStore *store){
	printfunc(__func__);

	sqlite3_stmt	 	*vm;
	char 				*sql_query = NULL;
	int					ret;

	sql_query = sqlite3_mprintf("SELECT serverID, desc FROM cardServer;");

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		printf("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return;
	}

	comboFlush(store);

	while(sqlite3_step(vm) != SQLITE_DONE){
		comboAppend(store, (gchar *) sqlite3_column_text(vm, 1), (guint) sqlite3_column_int(vm, 0));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
}
