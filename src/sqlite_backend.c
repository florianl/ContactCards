/*
 * sqlite_backend.c
 */

#include "ContactCards.h"

void doSimpleRequest(sqlite3 *ptr, char *sql_query, const char *func){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	int					ret;

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return;
	}

	ret = sqlite3_step(vm);

	if (ret != SQLITE_DONE){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return;
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);

}

void dbClose(sqlite3 *ptr){
	printfunc(__func__);

	sqlite3_close(ptr);
}

void dbCreate(sqlite3 *ptr){
	printfunc(__func__);

	char				**errmsg = NULL;
	int 				ret;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS contacts( \
	contactID INTEGER PRIMARY KEY AUTOINCREMENT,\
	addressbookID INTEGER, \
	etag TEXT, \
	href TEXT, \
	vCard TEXT, \
	displayname TEXT);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS oAuthServer( \
	oAuthID INTEGER PRIMARY KEY AUTOINCREMENT,\
	desc TEXT, \
	clientID TEXT, \
	clientSecret TEXT, \
	davURI TEXT, \
	scope TEXT, \
	grantURI TEXT, \
	tokenURI TEXT, \
	responseType TEXT, \
	redirURI TEXT, \
	grantType TEXT);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS cardServer( \
	serverID INTEGER PRIMARY KEY AUTOINCREMENT,\
	desc TEXT,\
	user TEXT, \
	passwd TEXT, \
	srvUrl TEXT,\
	authority TEXT,\
	resources INTEGER default 1,\
	isOAuth INTEGER default 0,\
	oAuthType INTEGER,\
	oAuthAccessGrant TEXT,\
	oAuthAccessToken TEXT,\
	oAuthRefreshToken TEXT);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS addressbooks( \
	addressbookID INTEGER PRIMARY KEY AUTOINCREMENT,\
	cardServer INTEGER, \
	displayname TEXT, \
	path TEXT, \
	syncToken TEXT, \
	syncMethod INTEGER);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	ret = sqlite3_exec(ptr, "CREATE TABLE IF NOT EXISTS certs( \
	certID INTEGER PRIMARY KEY AUTOINCREMENT,\
	serverID INTEGER, \
	trustFlag INTEGER default 2,\
	digest TEXT, \
	issued TEXT, \
	issuer TEXT, \
	cert TEXT);", NULL, NULL, errmsg);

	if (ret != SQLITE_OK)	goto sqlError;

	return;

sqlError:
	dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
	return;
}

GSList *multiCheck(sqlite3 *ptr, char *tableName, int rows, char *sel, char *row1, int value1, char *row2, char *value2, char *row3, char *value3){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int 				ret;
	GSList				*list = g_slist_alloc();

	g_strstrip(tableName);
	g_strstrip(sel);
	g_strstrip(row1);
	g_strstrip(row2);
	g_strstrip(value2);
	g_strstrip(row3);
	g_strstrip(value3);

	switch(rows){
		case 1:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d';", sel, tableName, row1, value1);
			break;
		case 2:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q';", sel, tableName, row2, value2);
			break;
		case 12:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q';", sel, tableName, row1, value1, row2, value2);
			break;
		case 23:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q' AND %q = '%q';", sel, tableName, row2, value2,  row3, value3);
			break;
		case 123:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q' AND %q = '%q';", sel, tableName, row1, value1, row2, value2, row3, value3);
			break;
		default:
			dbgCC("[%s] can't handle this number: %d\n", __func__, rows);
	}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return list;
	}

	while(sqlite3_step(vm) != SQLITE_DONE){
		dbgCC("[%s] append: %d\n", __func__, sqlite3_column_int(vm, 0));
		list = g_slist_append(list,  GINT_TO_POINTER(sqlite3_column_int(vm, 0)));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);

	return list;
}

/*
 * returns 0 if it does not exist
 */

int countElements(sqlite3 *ptr, char *tableName, int rows, char *row1, int value1, char *row2, char *value2, char *row3, char *value3){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int					count = 0;
	int 				ret;

	switch(rows){
		case 1:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%d';", tableName, row1, value1);
			break;
		case 2:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%q';", tableName, row2, value2);
			break;
		case 12:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%d' AND %q = '%q';", tableName, row1, value1, row2, value2);
			break;
		case 23:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%q' AND %q = '%q';", tableName, row2, value2,  row3, value3);
			break;
		case 123:
			sql_query = sqlite3_mprintf("SELECT COUNT(*) FROM %q WHERE %q = '%d' AND %q = '%q' AND %q = '%q';", tableName, row1, value1, row2, value2, row3, value3);
			break;
		default:
			dbgCC("[%s] can't handle this number: %d\n", __func__, rows);
	}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return 0;
	}

	while(sqlite3_step(vm) != SQLITE_DONE){
			count = sqlite3_column_int(vm, 0);
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);

	return count;
}

void setSingleInt(sqlite3 *ptr, char *tableName, char *setValue, int newValue, char *row1, int value1){
	printfunc(__func__);

	char 				*sql_query;

	sql_query = sqlite3_mprintf("UPDATE %q SET %q = '%d' WHERE %q = '%d';", tableName, setValue, newValue, row1, value1);

	doSimpleRequest(ptr, sql_query, __func__);
}

void setSingleChar(sqlite3 *ptr, char *tableName, char *setValue, char *newValue, char *row1, int value1){
	printfunc(__func__);

	char 				*sql_query;

	sql_query = sqlite3_mprintf("UPDATE %q SET %q = %Q WHERE %q = '%d';", tableName, setValue, newValue, row1, value1);

	doSimpleRequest(ptr, sql_query, __func__);
}

GSList *getListInt(sqlite3 *ptr, char *tableName, char *selValue, int selRow, char *row1, int value1, char *row2, char *value2){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int 				ret;
	GSList				*list = g_slist_alloc();

	switch(selRow){
		case 1:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d';", selValue, tableName, row1, value1);
			break;
		case 12:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2);
			break;
		case 2:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q';", selValue, tableName, row2, value2);
			break;
		case 0:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q;", selValue, tableName);
			break;
		default:
			dbgCC("[%s] can't handle this number: %d\n", __func__, selRow);
	}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return list;
	}

	while(sqlite3_step(vm) != SQLITE_DONE){
		list = g_slist_append(list,  GINT_TO_POINTER(sqlite3_column_int(vm, 0)));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);

	return list;

}

int getSingleInt(sqlite3 *ptr, char *tableName, char *selValue, int selRow, char *row1, int value1, char *row2, char *value2){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int					ret;
	int					count = 0;
	int					retValue = 0;

	switch(selRow){
		case 1:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d';", selValue, tableName, row1, value1);
			break;
		case 12:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2);
			break;
		case 2:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q';", selValue, tableName, row2, value2);
			break;
		default:
			dbgCC("[%s] can't handle this number: %d\n", __func__, selRow);
	}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return count;
	}

	while(sqlite3_step(vm) != SQLITE_DONE) {
		if(sqlite3_column_text(vm, 0) == NULL){
			retValue = 0;
		} else {
			retValue = sqlite3_column_int(vm, 0);
		}
		count++;
	}

	if(count != 1){
		dbgCC("[%s] there is more than one returning value. can't handle %d values\n", __func__, count);
		return -1;
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);

	return retValue;
}

char *getSingleChar(sqlite3 *ptr, char *tableName, char *selValue, int selRow, char *row1, int value1, char *row2, char *value2, char *row3, char *value3, char *row4, int value4){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int					ret;
	int					count = 0;
	char				*retValue = NULL;

	switch(selRow){
		case 1:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d';", selValue, tableName, row1, value1);
			break;
		case 12:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2);
			break;
		case 23:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%q' AND %q = '%q';", selValue, tableName, row2, value2, row3, value3);
			break;
		case 14:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%d';", selValue, tableName, row1, value1, row4, value4);
			break;
		case 123:
			sql_query = sqlite3_mprintf("SELECT %q FROM %q WHERE %q = '%d' AND %q = '%q' AND %q = '%q';", selValue, tableName, row1, value1, row2, value2, row3, value3);
			break;
		default:
			dbgCC("[%s] can't handle this number: %d\n", __func__, selRow);
	}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return NULL;
	}

	while(sqlite3_step(vm) != SQLITE_DONE) {
		if(sqlite3_column_text(vm, 0) == NULL){
			retValue = g_strndup(" ", 1);
		} else {
			retValue = g_strndup(sqlite3_column_text(vm, 0), strlen(sqlite3_column_text(vm, 0)));
		}
		count++;
	}

	if(count != 1){
		dbgCC("[%s] there is more than one returning value. can't handle %d values'\n", __func__, count);
		return NULL;
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);

	return retValue;
}

void newOAuthEntity(sqlite3 *ptr, char *desc, char *clientID, char *clientSecret, char *davURI, char *scope, char *grantURI, char *tokenURI, char *responseType, char *redirURI, char *grantType){
	printfunc(__func__);

	char		 		*sql_query;

	if(countElements(ptr, "oAuthServer", 2, "", 0, "desc", desc, "", "") !=0){
				return;
	}

	sql_query = sqlite3_mprintf("INSERT INTO oAuthServer (desc, clientID, clientSecret, davURI, scope , grantURI, tokenURI, responseType, redirURI, grantType) VALUES ('%q', '%q','%q', '%q','%q', '%q', '%q', '%q', '%q', '%q');", desc, clientID, clientSecret, davURI, scope, grantURI, tokenURI, responseType, redirURI, grantType);

	doSimpleRequest(ptr, sql_query, __func__);
}

void newServerOAuth(sqlite3 *ptr, char *desc, char *newuser, char *newGrant, int oAuthEntity){
	printfunc(__func__);

	char		 		*sql_query = NULL;
	char				*davBase = NULL;
	GSList				*retList;
	int					serverID;

	g_strstrip(desc);
	g_strstrip(newuser);
	g_strstrip(newGrant);

	davBase = getSingleChar(ptr, "oAuthServer", "davURI", 1, "oAuthID", oAuthEntity, "", "", "", "", "", 0);

	retList = multiCheck(ptr, "cardServer", 12, "serverID", "oAuthType", oAuthEntity, "user", newuser, "", "");

	if(g_slist_length(retList) != 1) {
		dbgCC("[%s] there is something similar\n", __func__);
		g_slist_free(retList);
		return;
	}
	g_slist_free(retList);

	sql_query = sqlite3_mprintf("INSERT INTO cardServer (desc, user, srvUrl, isOAuth, oAuthType) VALUES ('%q','%q','%q','%d','%d');", desc, newuser,  davBase, 1, oAuthEntity);

	doSimpleRequest(ptr, sql_query, __func__);

	serverID = getSingleInt(ptr, "cardServer", "serverID", 12, "oAuthType", oAuthEntity, "user", newuser);
	setSingleChar(ptr, "cardServer", "oAuthAccessGrant", newGrant, "serverID", serverID);

	oAuthAccess(ptr, serverID, oAuthEntity, DAV_REQ_GET_TOKEN);

	return;
}

void newServer(sqlite3 *ptr, char *desc, char *user, char *passwd, char *url){
	printfunc(__func__);

	ne_uri				uri;
	char		 		*sql_query;
	GSList				*retList;

	g_strstrip(desc);
	g_strstrip(user);
	g_strstrip(passwd);
	g_strstrip(url);

	ne_uri_parse(url, &uri);

	retList = multiCheck(ptr, "cardServer", 23, "serverID", "", 0, "user", user, "authority", uri.host);

	if(g_slist_length(retList) != 1){
		while(retList){
			GSList *next = retList->next;
			dbgCC("[%s] checking: %d\n", __func__, GPOINTER_TO_INT(retList->data));
			if(countElements(ptr, "addressbooks", 12, "cardServer", GPOINTER_TO_INT(retList->data), "path", uri.path, "", "") !=0){
				dbgCC("[%s] there is something similar\n", __func__);
				g_slist_free(retList);
				return;
			}
			if(countElements(ptr, "cardServer", 23, "", 0, "user", user, "srvUrl", url) !=0){
				dbgCC("[%s] there is something similar\n", __func__);
				g_slist_free(retList);
				return;
			}
			if(countElements(ptr, "cardServer", 23, "", 0, "user", user, "authority", uri.host) !=0){
				dbgCC("[%s] there is something similar\n", __func__);
				g_slist_free(retList);
				return;
			}
			retList = next;
		}
	}
	g_slist_free(retList);

	sql_query = sqlite3_mprintf("INSERT INTO cardServer (desc, user, passwd, srvUrl, authority) VALUES ('%q','%q','%q','%q', '%q');", desc, user, passwd, url, uri.host);

	doSimpleRequest(ptr, sql_query, __func__);

	fillCombo(ptr, comboList);
}


void showServer(sqlite3 *ptr){
	printfunc(__func__);

	sqlite3_stmt		*vm;

	sqlite3_prepare_v2(ptr, "SELECT * FROM cardServer", -1, &vm, NULL);
	while(sqlite3_step(vm) != SQLITE_DONE){
		printf("[%i]\t%s\t%s\n", sqlite3_column_int(vm, 0), sqlite3_column_text(vm, 1), sqlite3_column_text(vm, 4));
	}

	sqlite3_finalize(vm);
}

void showAddressbooks(sqlite3 *ptr){
	printfunc(__func__);

	sqlite3_stmt		*vm;

	sqlite3_prepare_v2(ptr, "SELECT * FROM addressbooks", -1, &vm, NULL);
	while(sqlite3_step(vm) != SQLITE_DONE){
		printf("[%i - %i]\t%s\t%s\t%s\n", sqlite3_column_int(vm, 0), sqlite3_column_int(vm, 1), sqlite3_column_text(vm, 2), sqlite3_column_text(vm, 3), sqlite3_column_text(vm, 4));
	}

	sqlite3_finalize(vm);
}

void cleanUpRequest(sqlite3 *ptr, int id, int type){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query = NULL;
	int					ret;

	switch(type){
		// addressbooks
		case 0:
				sql_query = sqlite3_mprintf("SELECT addressbookID FROM addressbooks WHERE cardServer = '%d';", id);
				break;
		// contacts
		case 1:
				sql_query = sqlite3_mprintf("SELECT contactID FROM contacts WHERE addressbookID = '%d';", id);
				break;
	}

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return;
	}

	while(sqlite3_step(vm) != SQLITE_DONE) {
		switch(type){
			// addressbooks
			case 0:
					cleanUpRequest(ptr, sqlite3_column_int(vm, 0), 1);
					dbRemoveItem(ptr, "addressbooks", 2, "", "", "addressbookID", sqlite3_column_int(vm, 0));
					break;
			// contacts
			case 1:
					dbRemoveItem(ptr, "contacts", 2, "", "", "contactID", sqlite3_column_int(vm, 0));
					break;
		}
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
}

void dbRemoveItem(sqlite3 *ptr, char *tableName, int selRow, char *row1, char *value1, char *row2, int value2){
	printfunc(__func__);

	char 				*sql_query;

	switch(selRow){
		case 1:
			sql_query = sqlite3_mprintf("DELETE FROM %q WHERE %q = '%q';", tableName, row1, value1);
			break;
		case 2:
			sql_query = sqlite3_mprintf("DELETE FROM %q WHERE %q = '%d';", tableName, row2, value2);
			break;
		case 12:
			sql_query = sqlite3_mprintf("DELETE FROM %q WHERE %q = '%q' AND %q = '%d';", tableName, row1, value1, row2, value2);
			break;
		default:
			dbgCC("[%s] can't handle this number: %d\n", __func__, selRow);
			return;
	}

	doSimpleRequest(ptr, sql_query, __func__);
}


void readCardServerCredits(int serverID, credits_t *key, sqlite3 *ptr){
	printfunc(__func__);

	sqlite3_stmt 		*vm;
	char 				*sql_query;
	int					ret;

	sql_query = sqlite3_mprintf("SELECT user, passwd FROM cardServer WHERE serverID = '%d';", serverID);

	ret = sqlite3_prepare_v2(ptr, sql_query, strlen(sql_query), &vm, NULL);

	if (ret != SQLITE_OK){
		dbgCC("[%s] %d - %s\n", __func__, sqlite3_extended_errcode(ptr), sqlite3_errmsg(ptr));
		return;
	}

	while(sqlite3_step(vm) != SQLITE_DONE) {
		key->user	= g_strdup((char *) sqlite3_column_text(vm, 0));
		key->passwd	= g_strdup((char *) sqlite3_column_text(vm, 1));
	}

	sqlite3_finalize(vm);
	sqlite3_free(sql_query);
}

/*
 * This is a simple try to fix the URI of a vCard
 * It is expected that only the basepath is broken
 */
static char *fixURI(char *base,char *corrupted){
	printfunc(__func__);

	char			*new = NULL;
	char			**elements = g_strsplit(corrupted, "/", 0);
	char			*tmp;
	int				i = 0;

	do{
		i++;
	} while(elements[i] != NULL);

	tmp = g_strdup(elements[i-1]);

	new = g_strconcat(base, tmp, NULL);

	g_free(tmp);
	g_strfreev(elements);

	return new;
}

void contactHandle(sqlite3 *ptr, char *href, char *etag, int serverID, int addressbookID, ne_session *sess){
	printfunc(__func__);

	char 				*sql_query;
	char				*basePath;
	int					contactID;

	basePath = getSingleChar(ptr, "addressbooks", "path", 1, "addressbookID", addressbookID, "", "", "", "", "", 0);
	if(strlen(basePath) == 1) return;
	if(strncmp(href, basePath, strlen(basePath))){
		char				*tmp = NULL;
		tmp = fixURI(basePath, href);
		if(strncmp(tmp, basePath, strlen(basePath))){
			return;
		} else {
			href = tmp;
		}
	}

	if(countElements(ptr, "contacts", 123, "addressbookID", addressbookID, "etag", etag, "href", href) != 0){
		return;
	}

	if(countElements(ptr, "contacts", 12, "addressbookID", addressbookID, "href", href, "", "") != 0){
		sql_query = sqlite3_mprintf("UPDATE contacts SET etag = %Q WHERE addressbookID = %d AND href = %Q;", etag, addressbookID, href);
	} else {
		sql_query = sqlite3_mprintf("INSERT INTO contacts (addressbookID, etag, href) VALUES ('%d', %Q, %Q);", addressbookID, etag, href);
	}

	doSimpleRequest(ptr, sql_query, __func__);

	contactID = getSingleInt(ptr, "contacts", "contactID", 12, "addressbookID", addressbookID, "href", href);

	if(contactID == -1) return;

	serverRequest(DAV_REQ_GET, serverID, contactID, sess, ptr);

}

void newAddressbook(sqlite3 *ptr, int cardServer, char *displayname, char *path){
	printfunc(__func__);

	char		 		*sql_query;

	if(countElements(ptr, "addressbooks", 123, "cardServer", cardServer, "displayname", displayname, "path", path) != 0){
		return;
	}

	sql_query = sqlite3_mprintf("INSERT INTO addressbooks (cardServer, displayname, path) VALUES ('%d','%q','%q');", cardServer, displayname, path);

	doSimpleRequest(ptr, sql_query, __func__);

}


void showContacts(sqlite3 *ptr){
	printfunc(__func__);

	sqlite3_stmt		*vm;

	sqlite3_prepare_v2(ptr, "SELECT * FROM contacts", -1, &vm, NULL);

	while(sqlite3_step(vm) != SQLITE_DONE)
	{
		printf("cID:%i\taID: %d\t%s\t%s\n", sqlite3_column_int(vm, 0), sqlite3_column_int(vm, 1), sqlite3_column_text(vm, 3), sqlite3_column_text(vm, 2));
	}

	sqlite3_finalize(vm);
}

void setDisplayname(sqlite3 *ptr, int contactID, char *vData){
	printfunc(__func__);

	char			*displayName = NULL;
	char	 		*sql_query = NULL;

	displayName = getSingleCardAttribut(CARDTYPE_FN, vData);

	if(strlen(g_strstrip(displayName)) == 0){
		sql_query = sqlite3_mprintf("UPDATE contacts SET displayname = '(no name)' WHERE contactID = '%d';", contactID);
	} else {
		sql_query = sqlite3_mprintf("UPDATE contacts SET displayname = %Q WHERE contactID = '%d';", g_strstrip(displayName), contactID);
	}
	doSimpleRequest(ptr, sql_query, __func__);
}

void updateServerDetails(sqlite3 *ptr, int srvID, const gchar *newDesc, const gchar *newUrl, const gchar *newUser, const gchar *newPw, gboolean certSel){

	char				*oldDesc = NULL, *oldUrl = NULL, *oldUser = NULL, *oldPw = NULL;

	oldDesc = getSingleChar(ptr, "cardServer", "desc", 1, "serverID", srvID, "", "", "", "", "", 0);
	oldUrl = getSingleChar(ptr, "cardServer", "srvUrl", 1, "serverID", srvID, "", "", "", "", "", 0);
	oldUser = getSingleChar(ptr, "cardServer", "user", 1, "serverID", srvID, "", "", "", "", "", 0);
	oldPw =  getSingleChar(ptr, "cardServer", "passwd", 1, "serverID", srvID, "", "", "", "", "", 0);

	if(g_strcmp0(oldDesc, newDesc))
		setSingleChar(ptr, "cardServer", "desc", (char *) newDesc, "serverID", srvID);
	if(g_strcmp0(oldUrl, newUrl))
		setSingleChar(ptr, "cardServer", "srvUrl", (char *) newUrl, "serverID", srvID);
	if(g_strcmp0(oldUser, newUser))
		setSingleChar(ptr, "cardServer", "user", (char *) newUser, "serverID", srvID);
	if(g_strcmp0(oldPw, newPw))
		setSingleChar(ptr, "cardServer", "passwd", (char *) newPw, "serverID", srvID);
	if(certSel == TRUE)
		setSingleInt(ptr, "certs", "trustFlag", (int) ContactCards_DIGEST_TRUSTED, "serverID", srvID);
	if(certSel == FALSE)
		setSingleInt(ptr, "certs", "trustFlag", (int) ContactCards_DIGEST_UNTRUSTED, "serverID", srvID);
}

void updateContact(sqlite3 *ptr, int contactID, char *vData){
	printfunc(__func__);

	char	 			*sql_query = NULL;
	char				*dbData = NULL;

	if(!g_str_has_prefix(vData, "BEGIN:VCARD")){
		dbRemoveItem(ptr, "contacts", 2, "", "", "contactID", contactID);
		return;
	}

	sql_query = sqlite3_mprintf("UPDATE contacts SET vCard = %Q WHERE contactID = '%d';", vData, contactID);

	doSimpleRequest(ptr, sql_query, __func__);

	dbData = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", contactID, "", "", "", "","",0);

	if(strlen(dbData) == 1) return;

	setDisplayname(ptr, contactID, dbData);
}

void updateUri(sqlite3 *ptr, int serverID, char *new, gboolean force){
	printfunc(__func__);

	char				*sql_query = NULL;
	char				*old = NULL;
	char				*uri = NULL;
	ne_uri				newUri;
	ne_uri				oldUri;

	old = getSingleChar(ptr, "cardServer", "srvUrl", 1, "serverID", serverID, "", "", "", "", "", 0);

	ne_uri_parse(new, &newUri);
	ne_uri_parse(old, &oldUri);

	if(force == FALSE){
		dbgCC("[%s] %s - %s\n", __func__, oldUri.path, newUri.path);
		if(strlen(oldUri.path) > strlen(newUri.path)){
			dbgCC("[%s] will not update URI\n", __func__);
			return;
		} else {
			dbgCC("[%s] update URI\n", __func__);
		}
	} else {
		dbgCC("[%s] %s%s - %s%s\n", __func__, oldUri.host, oldUri.path, newUri.host, newUri.path);
	}

	newUri.scheme = newUri.scheme ? newUri.scheme : "https";
	newUri.port = newUri.port ? newUri.port : ne_uri_defaultport(newUri.scheme);

	if(newUri.host == NULL){
		uri = g_strdup_printf("%s://%s:%d%s", newUri.scheme, oldUri.host, newUri.port, newUri.path);
	} else {
		uri = g_strdup_printf("%s://%s:%d%s", newUri.scheme, newUri.host, newUri.port, newUri.path);
	}

	sql_query = sqlite3_mprintf("UPDATE cardServer SET srvUrl = %Q WHERE serverID = '%d';", uri, serverID);

	doSimpleRequest(ptr, sql_query, __func__);
}

gboolean checkSyncToken(sqlite3 *ptr, int addressbookID){
	printfunc(__func__);

	char			*syncToken = NULL;

	syncToken = getSingleChar(ptr, "addressbooks", "syncToken", 1, "addressbookID", addressbookID, "", "", "", "", "", 0);

	if(strlen(syncToken) == 1) return FALSE;

	if(!strncmp(syncToken, " ", strlen(syncToken))){
		return FALSE;
	}

	return TRUE;
}

void checkAddressbooks(sqlite3 *ptr, int serverID, int type, ne_session *sess){
	printfunc(__func__);

	GSList			*retList;

	retList = getListInt(ptr, "addressbooks", "addressbookID", 1, "cardServer", serverID, "", "");

	while(retList) {
		GSList				*next = retList->next;
		int					addressbookID = GPOINTER_TO_INT(retList->data);
		int					syncType;
		ContactCards_stack_t		*stack = NULL;

		if(addressbookID == 0){
			retList = next;
			continue;
		}
		switch(type){
			case 10:
				/*
				 * check how the address book will be synced
				 */
				stack = serverRequest(DAV_REQ_ADDRBOOK_SYNC, serverID, addressbookID, sess, ptr);
				break;
			case 20:
				/*
				 * get the latest content of each address book
				 */
				syncType = getSingleInt(ptr, "addressbooks", "syncMethod", 1, "addressbookID", addressbookID, "", "");
				if(syncType == -1) return;
				if(syncType & DAV_SYNC_COLLECTION){
					if(!checkSyncToken(ptr, addressbookID)){
						stack = serverRequest(DAV_REQ_REP_1, serverID, addressbookID, sess, ptr);
					} else {
						stack = serverRequest(DAV_REQ_REP_2, serverID, addressbookID, sess, ptr);
					}
				} else {
					stack = serverRequest(DAV_REQ_REP_3, serverID, addressbookID, sess, ptr);
				}
				break;
			default:
				dbgCC("[%s] can't handle this number: %d\n", __func__, type);
				return;
		}
		responseHandle(stack, sess, ptr);
		retList = next;
	}

	g_slist_free(retList);
}

void updateSyncToken(sqlite3 *ptr, int addressbookID, char *syncToken){
	printfunc(__func__);

	char 				*sql_query;

	sql_query = sqlite3_mprintf("UPDATE addressbooks SET syncToken = '%q' WHERE addressbookID = '%d';", syncToken, addressbookID);

	doSimpleRequest(ptr, sql_query, __func__);
}

void setServerCert(sqlite3 *ptr, int serverID, int counter, int trustFlag, char *cert, char *digest, char *issued, char *issuer){
	printfunc(__func__);

	char 				*sql_query;

	g_strstrip(cert);
	g_strstrip(digest);
	g_strstrip(issued);
	g_strstrip(issuer);

	if(counter != 0){
		sql_query = sqlite3_mprintf("DELETE FROM certs WHERE serverID = '%d';", serverID);
		doSimpleRequest(ptr, sql_query, __func__);
	}

	sql_query = sqlite3_mprintf("INSERT INTO certs (serverID, trustFlag, digest, issued, issuer, cert) VALUES ('%d','%d','%q','%q','%q','%q');", serverID, trustFlag, digest, issued, issuer, cert);

	doSimpleRequest(ptr, sql_query, __func__);
}

void updateOAuthCredentials(sqlite3 *ptr, int serverID, int tokenType, char *value){
	printfunc(__func__);

	char 				*sql_query;

	g_strdelimit(value, ",", '\n');
	g_strstrip(value);

	switch(tokenType){
		case OAUTH_ACCESS_GRANT:
			sql_query = sqlite3_mprintf("UPDATE cardServer SET oAuthAccessGrant = '%q' WHERE serverID = '%d';", value, serverID);
			break;
		case OAUTH_ACCESS_TOKEN:
			sql_query = sqlite3_mprintf("UPDATE cardServer SET oAuthAccessToken = '%q' WHERE serverID = '%d';", value, serverID);
			break;
		case OAUTH_REFRESH_TOKEN:
			sql_query = sqlite3_mprintf("UPDATE cardServer SET oAuthRefreshToken = '%q' WHERE serverID = '%d';", value, serverID);
			break;
		default:
			return;
	}

	doSimpleRequest(ptr, sql_query, __func__);
}
