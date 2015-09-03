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

#include <glib/gstdio.h>
#include "contactcards.h"

void contactsTreeFill(GSList *contacts){
	__PRINTFUNC__;

	while(contacts){
		GSList				*next =  contacts->next;
		int					id = GPOINTER_TO_INT(contacts->data);
		int					flags = 0;
		char				*card = NULL;
		if(id == 0){
			contacts = next;
			continue;
		}

		flags = getSingleInt(appBase.db, "contacts", "flags", 1, "contactID", id, "", "", "", "");
		if((flags & CONTACTCARDS_TMP) == CONTACTCARDS_TMP){
			debugCC("[%s] hiding %d\n", __func__, id);
			contacts = next;
			continue;
		}

		card = getSingleChar(appBase.db, "contacts", "vCard", 1, "contactID", id, "", "", "", "", "", 0);
		contactsTreeAppend(card, id);
		g_free(card);
		contacts = next;
	}
}

/**
 * fillList - fill a list
 */
void fillList(sqlite3 *ptr, int type, int from, int id, GtkWidget *list){
	__PRINTFUNC__;

	sqlite3_stmt	 	*vm;
	char 				*sql_query = NULL;
	int					ret;

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

	while(sqlite3_mutex_try(dbMutex) != SQLITE_OK){}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		verboseCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		sqlite3_mutex_leave(dbMutex);
		return;
	}

	debugCC("[%s] %s\n", __func__, sql_query);

	while(sqlite3_step(vm) != SQLITE_DONE){
		listAppend(list, (gchar *) sqlite3_column_text(vm, 1), (guint) sqlite3_column_int(vm, 0));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
	sqlite3_mutex_leave(dbMutex);
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
	__PRINTFUNC__;

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
 * exportOneContact - exports one contact
 */
void exportOneContact(sqlite3 *ptr, int selID, char *base){
	__PRINTFUNC__;

	char				*path = NULL;
	char				*contactName = NULL;
	char				*contactFile = NULL;
	char				*contactCard = NULL;
	GError 				*error = NULL;

	debugCC("[%s] %d > %s\n", __func__, selID, base);

	path = g_strconcat(base, NULL);
	if(g_chdir(path)){
		return;
	}
	contactName = getSingleChar(appBase.db, "contacts", "displayname", 1, "contactID", selID, "", "", "", "", "", 0);
	contactFile = g_strconcat(contactName, ".vcf", NULL);
	contactCard = getSingleChar(appBase.db, "contacts", "vCard", 1, "contactID", selID, "", "", "", "", "", 0);
	g_build_filename(contactFile, NULL);
	g_file_set_contents(contactFile, contactCard, strlen(contactCard), &error);
	printGError(error);

	g_free(contactName);
	g_free(contactFile);
	g_free(contactCard);
	g_free(path);
}

/**
 * exportContactsAddrBook - Export contacts from address book
 */
void exportContactsAddrBook(sqlite3 *ptr, char *base, int addrbook){
	__PRINTFUNC__;

	GSList				*contactList;
	char				*path = NULL;
	char				*addrbookLoc = NULL;

	addrbookLoc = getSingleChar(appBase.db, "addressbooks", "displayname", 1, "addressbookID", addrbook, "", "", "", "", "", 0);
	if (!g_file_test(addrbookLoc, G_FILE_TEST_EXISTS)){
		if(g_mkdir(addrbookLoc, 0775)){
			verboseCC("[%s] failed to create a directory\n", __func__);
			return;
		}
	}

	path = g_strconcat(base, "/", addrbookLoc, NULL);
	contactList = getListInt(appBase.db, "contacts", "contactID", 1, "addressbookID", addrbook, "", "", "", "");
	while(contactList){
		GSList				*next = contactList->next;
		int					contactID = GPOINTER_TO_INT(contactList->data);

		if(contactID == 0){
			contactList = next;
			continue;
		}
		if(g_chdir(path)){
			if (!g_file_test(path, G_FILE_TEST_EXISTS)){
				if(g_mkdir(path, 0775)){
					verboseCC("[%s] failed to create a directory\n", __func__);
					return;
				}
			} else {
				return;
			}
		}
		exportOneContact(appBase.db, contactID, path);
		contactList = next;
	}
	g_free(addrbookLoc);
	g_free(path);
	g_slist_free(contactList);
}

/**
 * exportContactsFav - Export contacts marked as favorit
 */
void exportContactsFav(sqlite3 *ptr, char *base){
	__PRINTFUNC__;

	GSList				*contactList;
	char				*path = NULL;

	path = g_strconcat(base, "/Favorites", NULL);
	contactList = getListInt(appBase.db, "contacts", "contactID", 91, "flags", CONTACTCARDS_FAVORIT, "", "", "", "");
	while(contactList){
		GSList				*next = contactList->next;
		int					contactID = GPOINTER_TO_INT(contactList->data);

		if(contactID == 0){
			contactList = next;
			continue;
		}
		if(g_chdir(path)){
			if (!g_file_test(path, G_FILE_TEST_EXISTS)){
				if(g_mkdir(path, 0775)){
					verboseCC("[%s] failed to create a directory\n", __func__);
					return;
				}
			} else {
				return;
			}
		}
		exportOneContact(appBase.db, contactID, path);
		contactList = next;
	}
	g_free(path);
	g_slist_free(contactList);
}

/**
 * exportContactsLoc - Export local contacts
 */
void exportContactsLoc(sqlite3 *ptr, char *base){
	__PRINTFUNC__;

	GSList				*contactList;
	char				*path = NULL;

	path = g_strconcat(base, "/Favorites", NULL);
	contactList = getListInt(appBase.db, "contacts", "contactID", 91, "flags", CONTACTCARDS_LOCAL, "", "", "", "");
	while(contactList){
		GSList				*next = contactList->next;
		int					contactID = GPOINTER_TO_INT(contactList->data);

		if(contactID == 0){
			contactList = next;
			continue;
		}
		if(g_chdir(path)){
			if (!g_file_test(path, G_FILE_TEST_EXISTS)){
				if(g_mkdir(path, 0775)){
					verboseCC("[%s] failed to create a directory\n", __func__);
					return;
				}
			} else {
				return;
			}
		}
		exportOneContact(appBase.db, contactID, path);
		contactList = next;
	}
	g_free(path);
	g_slist_free(contactList);
}

/**
 * exportContactsSrv - Export contacts from server
 */
void exportContactsSrv(sqlite3 *ptr, char *base, int srv){
	__PRINTFUNC__;

	GSList				*addressbookList;
	char				*path = NULL;
	char				*serverLoc = NULL;

	serverLoc = getSingleChar(appBase.db, "cardServer", "desc", 1, "serverID", srv, "", "", "", "", "", 0);
	if (!g_file_test(serverLoc, G_FILE_TEST_EXISTS)){
			if(g_mkdir(serverLoc, 0775)){
				verboseCC("[%s] failed to create a directory\n", __func__);
				return;
			}
	}
	path = g_strconcat(base, "/", serverLoc, NULL);
	if(g_chdir(path)){
		if (!g_file_test(path, G_FILE_TEST_EXISTS)){
			if(g_mkdir(path, 0775)){
				verboseCC("[%s] failed to create a directory\n", __func__);
				return;
			}
		}
	}

	addressbookList = getListInt(appBase.db, "addressbooks", "addressbookID", 1, "cardServer", srv, "", "", "", "");

	while(addressbookList){
		GSList				*next = addressbookList->next;
		int					addrbookID = GPOINTER_TO_INT(addressbookList->data);
		if(addrbookID == 0){
			addressbookList = next;
			continue;
		}
		if(g_chdir(path)){
			return;
		}
		exportContactsAddrBook(ptr, path, addrbookID);
		addressbookList = next;
	}
	g_free(path);
	g_free(serverLoc);
	g_slist_free(addressbookList);
}

/**
 * exportContactsAll - exports all contacts
 */
void exportContactsAll(sqlite3 *ptr, char *base){
	__PRINTFUNC__;

	GSList				*serverList;
	char				*path = NULL;

	path = g_strconcat(base, NULL);
	if(g_chdir(path)){
		return;
	}
	serverList = getListInt(appBase.db, "cardServer", "serverID", 0, "", 0, "", "", "", "");
	while(serverList){
		GSList				*next = serverList->next;
		int					serverID = GPOINTER_TO_INT(serverList->data);

		if(serverID == 0){
			serverList = next;
			continue;
		}
		if(g_chdir(path)){
			return;
		}
		exportContactsSrv(ptr, path, serverID);
		serverList = next;
	}
	g_free(path);
	g_slist_free(serverList);
}

/**
 * exportContactsCB - Callback to export contacts
 */
void exportContactsCB(sqlite3 *ptr, char *base, int type, int sel){
	__PRINTFUNC__;

	debugCC("[%s] %d|%d to %s\n", __func__, type, sel, base);

	switch(type){
		case 0:			/*	All				*/
			exportContactsAll(ptr, base);
			break;
		case 1:			/*	Server			*/
			exportContactsSrv(ptr, base, sel);
			break;
		case 2:			/*	Address book	*/
			exportContactsAddrBook(ptr, base, sel);
			break;
		case 3:
			exportOneContact(ptr, sel, base);
			break;
		case 4:
			exportContactsFav(ptr, base);
			break;
		case 5:
			exportContactsLoc(ptr, base);
			break;
		default:
			break;
	}

}

/**
 * writeCalendarHead - write the header for the .ics file
 */
void writeCalendarHead(int fd){
	__PRINTFUNC__;

	GString				*content;

	content = g_string_new(NULL);

	g_string_append(content, "BEGIN:VCALENDAR\r\n");
	g_string_append(content, "VERSION:2.0\r\n");
	g_string_append(content, "PRODID:-//ContactCards//ContactCards");
	g_string_append(content, VERSION);
	g_string_append(content, "//EN\r\n");

	write(fd, content->str, content->len);
	g_string_free(content, TRUE);
}

/**
 * writeEvent - write a single event into a .ics file
 */
void writeEvent(int fd, GDate *date, char *card){
	__PRINTFUNC__;

	GString				*content;
	char				*stamp = NULL,
						*fn = NULL;

	content = g_string_new(NULL);

	fn = getSingleCardAttribut(CARDTYPE_FN, card);
	stamp = g_strdup_printf("%04d%02d%02d", g_date_get_year(date), g_date_get_month(date), g_date_get_day(date));

	/*
	* RFC 5545 - 3.6.1 Event Component
	*/
	g_string_append(content,"BEGIN:VEVENT\r\n");
	g_string_append(content,"DTSTAMP:");
	g_string_append(content, stamp);
	g_string_append(content,"T000001Z");
	g_string_append(content,"\r\n");
	g_string_append(content,"UID:");
	g_string_append(content, getUID());
	g_string_append(content, "-ContactCards");
	g_string_append(content, VERSION);
	g_string_append(content, "\r\n");
	g_string_append(content,"DTSTART;VALUE=DATE:");
	g_string_append(content, stamp);
	g_string_append(content,"\r\n");
	g_string_append(content,"SUMMARY:");
	g_string_append(content,_("Birthday of "));
	g_string_append(content,fn);
	g_string_append(content,"\r\n");
	g_string_append(content,"TRANSP:TRANSPARENT\r\n");
	g_string_append(content,"RRULE:FREQ=YEARLY\r\n");
	g_string_append(content,"END:VEVENT\r\n");

	write(fd, content->str, content->len);
	g_free(stamp);
	g_free(fn);
	g_string_free(content, TRUE);
}

/**
 * writeEvents - write the events into the .ics file
 */
void writeEvents(int fd, GSList *contacts){
	__PRINTFUNC__;

	while(contacts){
		GSList				*next =  contacts->next;
		int					id = GPOINTER_TO_INT(contacts->data);
		int					flags = 0;
		char				*card = NULL;
		char				*bday = NULL;

		if(id == 0){
			contacts = next;
			continue;
		}

		flags = getSingleInt(appBase.db, "contacts", "flags", 1, "contactID", id, "", "", "", "");
		if((flags & CONTACTCARDS_TMP) == CONTACTCARDS_TMP){
			debugCC("[%s] hiding %d\n", __func__, id);
			contacts = next;
			continue;
		}

		card = getSingleChar(appBase.db, "contacts", "vCard", 1, "contactID", id, "", "", "", "", "", 0);

		bday = getSingleCardAttribut(CARDTYPE_BDAY, card);
		if(bday != NULL){
			GDate		*date;
			date = g_date_new();
			g_date_set_parse(date, bday);
			if(g_date_valid(date) == TRUE){
				writeEvent(fd, date, card);
			}
			g_date_free(date);
			g_free(bday);
			bday = NULL;
		}
		g_free(card);
		contacts = next;
	}
}

/**
 * writeCalendarFoot - write the footer for the .ics file
 */
void writeCalendarFoot(int fd){
	__PRINTFUNC__;
	write(fd, "END:VCALENDAR\r\n", sizeof("END:VCALENDAR\r\n"));
}

void exportBirthdays(int type, int id, char *base){
	__PRINTFUNC__;

	GError			*error = NULL;
	GSList			*contacts = NULL;
	char			*dst = NULL;
	char			*desc = NULL,
					*file = NULL;
	int				fd = 0;

	switch(type){
		case 0:		/*	server			*/
			desc = getSingleChar(appBase.db, "cardServer", "desc", 1, "serverID", id, "", "", "", "", "", 0);
			break;
		case 1:		/*	address book	*/
			desc = getSingleChar(appBase.db, "addressbooks", "displayname", 1, "addressbookID", id, "", "", "", "", "", 0);
			break;
		case 2:		/*	favorites		*/
			desc = g_strdup(_("Favorites"));
			break;
		case 3:
			desc = g_strdup(_("Locales"));
			break;
		default:
			return;
	}

	file = g_strconcat(desc, "_", _("Birthdays"), ".ics", NULL);
	dst = g_build_filename(base, file, NULL);
	g_free(desc);

	if(g_file_test(dst, G_FILE_TEST_EXISTS) == TRUE){
		verboseCC("%s exists already\n", dst);
	}

	fd = g_open(dst, O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		g_free(desc);
		g_free(file);
		g_free(dst);
		return;
	}

	writeCalendarHead(fd);

	switch(type){
		case 0:		/*	server selected	*/
			if(id == 0){
				contacts = getListInt(appBase.db, "contacts", "contactID", 0, "", 0, "", "", "", "");
				break;
			} else {
				GSList			*addressBooks;
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
					contacts = getListInt(appBase.db, "contacts", "contactID", 1, "addressbookID", addressbookID, "", "", "", "");
					writeEvents(fd, contacts);
					g_slist_free(contacts);
					addressBooks = next;
				}
				writeCalendarFoot(fd);
				g_close(fd, &error);
				if(error){
					verboseCC("[%s] something has gone wrong\n", __func__);
					verboseCC("%s\n", error->message);
				}
				g_slist_free(addressBooks);
				return;
			}
			break;
		case 1:		/*	address book selected	*/
			contacts = getListInt(appBase.db, "contacts", "contactID", 1, "addressbookID", id, "", "", "", "");
			break;
		case 2:		/*	favorites		*/
			contacts = getListInt(appBase.db, "contacts", "contactID", 91, "flags", CONTACTCARDS_FAVORIT, "", "", "", "");
			break;
		case 3:		/*	locales		*/
			contacts = getListInt(appBase.db, "contacts", "contactID", 91, "flags", CONTACTCARDS_LOCAL, "", "", "", "");
			break;
		default:
			break;
	}
	writeEvents(fd, contacts);
	writeCalendarFoot(fd);
	g_close(fd, &error);
	if(error){
		verboseCC("[%s] something has gone wrong\n", __func__);
		verboseCC("%s\n", error->message);
	}
	g_slist_free(contacts);
	return;
}
