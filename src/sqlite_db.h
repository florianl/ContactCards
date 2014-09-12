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

#ifndef sqlite_H
#define sqlite_H

#ifndef SQLITE3_LIB_H
#define SQLITE3_LIB_H
#include <sqlite3.h>
#endif	/*	SQLITE3_LIB_H	*/

#include "neon.h"

/*		sqlite_backend.c		*/
extern void showAddressbooks(sqlite3 *ptr);
extern void showContacts(sqlite3 *ptr);
extern void showServer(sqlite3 *ptr);
extern int countElements(sqlite3 *ptr, char *tableName, int rows, char *row1, int value1, char *row2, char *value2, char *row3, char *value3);
extern void checkAddressbooks(sqlite3 *ptr, int serverID, int type, ne_session *sess);
extern void cleanUpRequest(sqlite3 *ptr, int id, int type);
extern void dbCreate(sqlite3 *ptr);
extern void dbClose(sqlite3 *ptr);
extern char *getSingleChar(sqlite3 *ptr, char *tableName, char *selValue, int selRow, char *row1, int value1, char *row2, char *value2, char *row3, char *value3, char *row4, int value4);
extern int getSingleInt(sqlite3 *ptr, char *tableName, char *selValue, int selRow, char *row1, int value1, char *row2, char *value2, char *row3, char *value3);
extern GSList *getListInt(sqlite3 *ptr, char *tableName, char *selValue, int selRow, char *row1, int value1, char *row2, char *value2, char *row3, char *value3);
extern void handleServerOptions(char *val, int serverID);
extern void newAddressbook(sqlite3 *ptr, int cardServer, char *displayname, char *path);
extern int newAddressbookTmp(int srvID, char *name);
extern int newContact(sqlite3 *ptr, int addressbookID, char *card);
extern void newServer(sqlite3 *ptr, char *desc, char *user, char *passwd, char *url);
extern void newServerOAuth(sqlite3 *ptr, char *desc, char *newuser, char *newGrant, int oAuthEntity);
extern void newOAuthEntity(sqlite3 *ptr, char *desc, char *clientID, char *clientSecret, char *davURI, char *scope, char *grantURI, char *tokenURI, char *responseType, char *redirURI, char *grantType);
extern void contactHandle(sqlite3 *ptr, char *href, char *etag, int serverID, int addressbookID, ne_session *sess);
extern void readCardServerCredits(int serverID, credits_t *key, sqlite3 *ptr);
extern void remove_all_request(sqlite3 *ptr, char *tableName);
extern void dbRemoveItem(sqlite3 *ptr, char *tableName, int selRow, char *row1, char *value1, char *row2, int value2);
extern void setDisplayname(sqlite3 *ptr, int contactID, char *vData);
extern void setServerCert(sqlite3 *ptr, int serverID, int counter, int trustFlag, char *cert, char *digest, char *issued, char *issuer);
extern void setSingleChar(sqlite3 *ptr, char *tableName, char *setValue, char *newValue, char *row1, int value1);
extern void setSingleInt(sqlite3 *ptr, char *tableName, char *setValue, int newValue, char *row1, int value1);
extern void sync_addressbook(sqlite3 *ptr, int serverID);
extern void updateAddressbooks(sqlite3 *ptr, GSList *list);
extern void updateContact(sqlite3 *ptr, int contactID, char *vData);
extern void updateContactUri(sqlite3 *ptr, int contactID, char *uri);
extern void updateOAuthCredentials(sqlite3 *ptr, int serverID, int tokenType, char *value);
extern void updatePostURI(sqlite3 *ptr, int addressbookID, char *href);
extern void updateUri(sqlite3 *ptr, int serverID, char *newPath, gboolean force);
extern void updateServerDetails(sqlite3 *ptr, int srvID, const gchar *newDesc, const gchar *newUrl, const gchar *newUser, const gchar *newPw, gboolean certSel, gboolean syncSel);
extern void updateSyncToken(sqlite3 *ptr, int addressbookID, char *syncToken);

/*		sqlite_frontend.c		*/
extern void fillList(sqlite3 *ptr, int type, int from, int id, GtkWidget *list);
extern void exportBirthdays(int type, int id, char *base);
extern void exportContacts(sqlite3 *ptr, char *base);
extern void exportOneContact(int selID, char *path);
extern void exportCert(sqlite3 *ptr, char *base, int serverID);
extern void contactsTreeFill(GSList *contacts);

#endif	/*	sqlite_H	*/
