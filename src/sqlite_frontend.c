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
			dbgCC("[%s] can't handle this type: %d\n", __func__, type);
			return;
	}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
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
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return;
	}

	comboFlush(store);

	while(sqlite3_step(vm) != SQLITE_DONE){
		comboAppend(store, (gchar *) sqlite3_column_text(vm, 1), (guint) sqlite3_column_int(vm, 0));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
}

void printGError(GError *error){
	if (error != NULL) {
		printf("[%s] %s \n", __func__, error->message);
		g_clear_error (&error);
	}
}

void exportContacts(sqlite3 *ptr, char *base){
	printfunc(__func__);

	GSList				*serverList;
	GSList				*addressbookList;
	GSList				*contactList;
	char				*path;
	GError 				*error = NULL;

	serverList = getListInt(ptr, "cardServer", "serverID", 0, "", 0, "", "");
	while(serverList){
		GSList				*next = serverList->next;
		int					serverID = GPOINTER_TO_INT(serverList->data);
		char				*serverLoc;
		if(serverID == 0){
			serverList = next;
			continue;
		}
		path = g_strconcat(base, NULL);
		if(g_chdir(path)){
			return;
		}
		serverLoc = getSingleChar(ptr, "cardServer", "desc", 1, "serverID", serverID, "", "", "", "", "", 0);
		if (!g_file_test(serverLoc, G_FILE_TEST_EXISTS)){
			g_mkdir(serverLoc, 0775);
		}
		addressbookList = getListInt(ptr, "addressbooks", "addressbookID", 1, "cardServer", serverID, "", "");
		while(addressbookList){
			GSList				*next = addressbookList->next;
			int					addrbookID = GPOINTER_TO_INT(addressbookList->data);
			char				*addrbookLoc;
			if(addrbookID == 0){
				addressbookList = next;
				continue;
			}
			path = g_strconcat(base, "/", serverLoc, NULL);
			if(g_chdir(path)){
				return;
			}
			addrbookLoc = getSingleChar(ptr, "addressbooks", "displayname", 1, "addressbookID", addrbookID, "", "", "", "", "", 0);
			if (!g_file_test(addrbookLoc, G_FILE_TEST_EXISTS)){
				g_mkdir(addrbookLoc, 0775);
			}
			contactList = getListInt(ptr, "contacts", "contactID", 1, "addressbookID", addrbookID, "", "");
			while(contactList){
				GSList				*next = contactList->next;
				int					contactID = GPOINTER_TO_INT(contactList->data);
				char				*contactName = NULL;
				char				*contactFile = NULL;
				char				*contactCard = NULL;
				if(contactID == 0){
					contactList = next;
					continue;
				}
				path = g_strconcat(base, "/", serverLoc, "/", addrbookLoc, NULL);
				if(g_chdir(path)){
					return;
				}
				contactName = getSingleChar(ptr, "contacts", "displayname", 1, "contactID", contactID, "", "", "", "", "", 0);
				contactFile = g_strconcat(contactName, ".vcf", NULL);
				contactCard = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", contactID, "", "", "", "", "", 0);
				g_build_filename(contactFile, NULL);
				g_file_set_contents(contactFile, contactCard, strlen(contactCard), &error);
				printGError(error);
				contactList = next;
			}
			g_slist_free(contactList);
			addressbookList = next;
		}
		g_slist_free(addressbookList);
		serverList = next;
	}
	g_slist_free(serverList);
}
