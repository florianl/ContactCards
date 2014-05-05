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

#include "contactcards.h"

/**
 * fillList - fill a list
 */
void fillList(sqlite3 *ptr, int type, int from, int id, GtkWidget *list){
	printfunc(__func__);

	sqlite3_stmt	 	*vm;
	char 				*sql_query = NULL;
	int					ret;
	int					active = 0;

	listFlush(list);

	switch(type){
		case 2:		// contactlist
			if(from == 0 && id == 0){
				sql_query = sqlite3_mprintf("SELECT contactID, displayname FROM contacts;");
			} else if(from == 0 && id != 0){
				GSList				*addressBooks;
				char				*query = NULL,
									*tmp = NULL,
									*tmp2 = NULL;
				int					first = 0;

				query = g_strndup("SELECT contactID, displayname FROM contacts WHERE", strlen("SELECT contactID, displayname FROM contacts WHERE"));

				addressBooks = getListInt(appBase.db, "addressbooks", "addressbookID", 1, "cardServer", id, "", "", "", "");
				while(addressBooks){
					GSList				*next =  addressBooks->next;
					int					addressbookID = GPOINTER_TO_INT(addressBooks->data);
					int					active = 0;
					if(addressbookID == 0){
						addressBooks = next;
						continue;
					}
					active = getSingleInt(appBase.db, "addressbooks", "syncMethod", 1, "addressbookID", addressbookID, "", "", "", "");
					if(active & (1<<DAV_ADDRBOOK_DONT_SYNC)){
						/* Ignore address books which are not synced	*/
						addressBooks = next;
						continue;
					}
					tmp = g_strndup(query, strlen(query));
					if(first == 0){
						tmp2 = g_strdup_printf (" addressbookID = '%d'", addressbookID);
						first = 1;
					} else {
						tmp2 = g_strdup_printf (" OR addressbookID = '%d'", addressbookID);
					}
					query = g_strconcat(tmp, tmp2, NULL);
					addressBooks = next;
				}

				sql_query = sqlite3_mprintf("%s;", query);
				g_slist_free(addressBooks);
				g_free(tmp);
				g_free(tmp2);
				g_free(query);
			} else {
				sql_query = sqlite3_mprintf("SELECT contactID, displayname FROM contacts WHERE addressbookID = '%d';", from);
			}
			break;
		case 3:		// serverlist
			sql_query = sqlite3_mprintf("SELECT serverID, desc FROM cardServer;");
			break;
		default:
			verboseCC("[%s] can't handle this type: %d\n", __func__, type);
			return;
	}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		verboseCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return;
	}

	debugCC("[%s] %s\n", __func__, sql_query);

	while(sqlite3_step(vm) != SQLITE_DONE){
		if(type == 1){
			active = getSingleInt(ptr, "addressbooks", "syncMethod", 1, "addressbookID", sqlite3_column_int(vm, 0), "", "", "", "");
			if(active & (1<<DAV_ADDRBOOK_DONT_SYNC)){
				/* Don't display address books which are not synced	*/
				continue;
			}
		}
		listAppend(list, (gchar *) sqlite3_column_text(vm, 1), (guint) sqlite3_column_int(vm, 0));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
}

/**
 * printGError - print a error
 */
void printGError(GError *error){
	if (error != NULL) {
		printf("[%s] %s \n", __func__, error->message);
		g_clear_error (&error);
	}
}

/**
 * exportCert - export the certificate from a server
 */
void exportCert(sqlite3 *ptr, char *base, int serverID){
	printfunc(__func__);

	char				*fileName = NULL;
	char				*serverDesc = NULL;
	GString				*cert = NULL;
	char				*path = NULL;
	char				*certdata = NULL;
	GError				*error = NULL;
	unsigned int		i = 0;

	serverDesc = getSingleChar(ptr, "cardServer", "desc", 1, "serverID", serverID, "", "", "", "", "", 0);
	fileName = g_strconcat(serverDesc, ".pem", NULL);
	certdata = getSingleChar(ptr, "certs", "cert", 1, "serverID", serverID, "", "", "", "", "", 0);
	if(!serverDesc || !certdata || !fileName) return;

	cert = g_string_new("-----BEGIN CERTIFICATE-----\n");

	g_string_append(cert, certdata);

	for(i=0; (28+(1+i)*64+i) < cert->len; i++){
		cert = g_string_insert(cert, 28+(1+i)*64+i, "\n");
	}
	g_string_append(cert, "\n-----END CERTIFICATE-----\n");

	path = g_strconcat(base, NULL);
	if(g_chdir(path)) return;

	g_build_filename(fileName, NULL);
	g_file_set_contents(fileName, cert->str, cert->len, &error);
	printGError(error);
	g_string_free(cert, TRUE);

	g_free(serverDesc);
	g_free(certdata);
	g_free(path);
	g_free(fileName);
}

/**
 * exportContacts - exports all contacts of a server
 */
void exportContacts(sqlite3 *ptr, char *base){
	printfunc(__func__);

	GSList				*serverList;
	GSList				*addressbookList;
	GSList				*contactList;
	char				*path = NULL;
	GError 				*error = NULL;

	serverList = getListInt(ptr, "cardServer", "serverID", 0, "", 0, "", "", "", "");
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
		addressbookList = getListInt(ptr, "addressbooks", "addressbookID", 1, "cardServer", serverID, "", "", "", "");
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
			contactList = getListInt(ptr, "contacts", "contactID", 1, "addressbookID", addrbookID, "", "", "", "");
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
		g_free(serverLoc);
	}
	g_slist_free(serverList);
	g_free(path);
}
