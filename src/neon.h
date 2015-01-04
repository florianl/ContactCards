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

#ifndef	neon_H
#define neon_H

#include <ne_session.h>

/*
 * Sync-Methods
 */
#define	DAV_SYNC_MASK					0x00F
#define	DAV_SYNC_COLLECTION				0x001
#define	DAV_ADDRBOOK_MULTIGET			0x002
#define	DAV_ADDRBOOK_QUERY				0x004
#define	DAV_ADDRBOOK_DONT_SYNC			0x008

/*
 * Server Options
 */
#define	DAV_OPT_MASK					0xFF0
#define	DAV_OPT_POST					0x800
#define	DAV_OPT_PUT						0x400
#define	DAV_OPT_DELETE					0x200
#define	DAV_OPT_MKCOL					0x100
#define	DAV_OPT_PROPPATCH				0x080
#define	DAV_OPT_MOVE					0x040
#define	DAV_OPT_REPORT					0x020
#define	DAV_OPT_ADDRESSBOOK				0x010

/*
 * Request-Methods
 */
#define	DAV_REQ_EMPTY					101
#define	DAV_REQ_OPTIONS					102

#define	DAV_REQ_CUR_PRINCIPAL			111
#define	DAV_REQ_ADDRBOOK_HOME			112
#define	DAV_REQ_ADDRBOOKS				113
#define	DAV_REQ_ADDRBOOK_SYNC			114

#define	DAV_REQ_REP_1					131
#define	DAV_REQ_REP_2					132
#define	DAV_REQ_REP_3					133

#define DAV_REQ_DEL_CONTACT				141
#define DAV_REQ_DEL_COLLECTION			142

#define	DAV_REQ_PUT_CONTACT				151
#define	DAV_REQ_POST_CONTACT			152
#define	DAV_REQ_POST_URI				153
#define	DAV_REQ_PUT_NEW_CONTACT			154

#define	DAV_REQ_GET						161

#define	DAV_REQ_GET_GRANT				171
#define	DAV_REQ_GET_TOKEN				172
#define	DAV_REQ_GET_REFRESH				173

#define	DAV_REQ_NEW_COLLECTION			181

/*
 * OAuth-Elements
 */
#define	OAUTH_ACCESS_GRANT				200
#define	OAUTH_ACCESS_TOKEN				201
#define	OAUTH_REFRESH_TOKEN				202
#define	OAUTH_TOKEN_TYPE				203
#define	OAUTH_EXPIRES_IN				204

#define	OAUTH_GRANT_FAILURE				210
#define	OAUTH_ACCESSTOKEN_FAILURE		211
#define	OAUTH_REFRESHTOKEN_FAILURE		212
#define	OAUTH_UP2DATE					213

/*
 * General stuff
 */
#define	BUFFERSIZE						8192
#define	DAV_XML_HEAD					"<?xml version=\"1.0\" encoding=\"utf-8\" ?>"

/*
 * PROPFIND
 */
#define	DAV_PROPFIND_START				"<d:propfind xmlns:d=\"DAV:\">"
#define	DAV_PROPFIND_END				"</d:propfind>"
#define	DAV_PROP_START					"<d:prop>"
#define	DAV_PROP_END					"</d:prop>"
#define	DAV_CUR_PRINCIPAL				"<d:current-user-principal />"
#define	DAV_ADDRBOOK_HOME				"<e:addressbook-home-set xmlns:e=\"urn:ietf:params:xml:ns:carddav\" /><d:resourcetype/>"
#define	DAV_ADDRBOOK_SETTING			"<d:allprop />"
#define	DAV_ADDRBOOKS					"<d:displayname /><d:resourcetype/>"
#define	DAV_ADDRBOOK_SYNC				"<d:supported-report-set/>"

#define	DAV_PRINCIPAL_SEARCH_START		"<d:principal-property-search xmlns:d=\"DAV:\">"
#define	DAV_PRINCIPAL_SEARCH_END		"</d:principal-property-search>"

#define	DAV_POST_URI					"<d:add-member/>"

#define	DAV_REP_SYNC_START				"<d:sync-collection xmlns:d=\"DAV:\">"
#define	DAV_REP_SYNC_TOKEN				"<d:sync-token />"
#define	DAV_REP_SYNC_TOKEN_START		"<d:sync-token>"
#define	DAV_REP_SYNC_TOKEN_END			"</d:sync-token>"
#define	DAV_REP_ETAG					"<d:getetag />"
#define	DAV_REP_SYNC_END				"</d:sync-collection>"

#define	DAV_MKCOL_START					"<d:mkcol xmlns:d=\"DAV:\">"
#define	DAV_MKCOL_END					"</d:mkcol>"
#define	DAV_SET_START					"<d:set>"
#define	DAV_SET_END						"</d:set>"
#define	DAV_RESOURCETYPE_START			"<d:resourcetype>"
#define	DAV_RESOURCETYPE_END			"</d:resourcetype>"
#define	DAV_COLLECTION					"<d:collection/>"
#define	DAV_ADDRESSBOOK					"<e:addressbook xmlns:e=\"urn:ietf:params:xml:ns:carddav\" />"
#define	DAV_DISPLAYNAME_START			"<d:displayname>"
#define	DAV_DISPLAYNAME_END				"</d:displayname>"

/*
 * DAV-Elementtypes
 */
#define	DAV_ELE_HREF					801
#define	DAV_ELE_DISPLAYNAME				802
#define	DAV_ELE_STATUS_200				803
#define	DAV_ELE_ADDRESSBOOK				804
#define	DAV_ELE_ETAG					805
#define	DAV_ELE_SYNCTOKEN				806
#define	DAV_ELE_ADDRBOOK_MULTIGET		807
#define	DAV_ELE_ADDRBOOK_QUERY			808
#define	DAV_ELE_SYNC_COLLECTION			809
#define	DAV_ELE_ADDRBOOK_HOME			810
#define	DAV_ELE_CUR_PRINCIPAL			811
#define	DAV_ELE_PROXY					812
#define	DAV_ELE_ADD_MEMBER				813

/*
 * Certificate stuff
 */
#define	ContactCards_DIGEST_TRUSTED		0
#define	ContactCards_DIGEST_NEW			1
#define	ContactCards_DIGEST_UNTRUSTED	2

/**
 * struct ContactCards_stack - contains the response to an request
 */
typedef struct ContactCards_stack {
	GNode			*tree;
	GNode			*lastBranch;
	int				serverID;
	int				addressbookID;
	int				statuscode;
	int				reqMethod;
} ContactCards_stack_t;

/**
 * struct ContactCards_stack - contains a subtree of the xml-response
 */
typedef struct ContactCards_node {
	char			*name;
	char			*content;
} ContactCards_node_t;

/**
 * struct ContactCards_leaf -  contains the final data of the response
 */
typedef struct ContactCards_leaf {
	char			*status;
	char			*contentType;
	char			*href;
	char			*etag;
} ContactCards_leaf_t;

/**
 * struct credits - contains the credentials of a user
 */
typedef struct credits{
	char			*user;
	char			*passwd;
} credits_t;

extern gboolean validateUrl(char *url);
extern int serverConnectionTest(int serverID);
extern ne_session *serverConnect(int serverID);
extern ContactCards_stack_t *serverRequest(int method, int serverID, int itemID, ne_session *sess, sqlite3 *ptr);
extern void serverDisconnect(ne_session *sess, sqlite3 *ptr, int serverID);
extern void responseHandle(ContactCards_stack_t *stack, ne_session *sess, sqlite3 *ptr);
extern void oAuthAccess(sqlite3 *ptr, int serverID, int oAuthServerEntity, int type);
extern int oAuthUpdate(sqlite3 *ptr, int serverID);
extern int pushCard(sqlite3 *ptr, char *card, int addrBookID, int existing, int oldID);
extern int responseOAuthHandle(void *data, const char *block, size_t len);
extern int serverCreateCollection(ne_session *sess, int srvID, char *colName);
extern int serverDelContact(sqlite3 *ptr, ne_session *sess, int serverID, int selID);
extern int serverDelCollection(sqlite3 *ptr, ne_session *sess, int serverID, int selID);
extern void syncContacts(sqlite3 *ptr, ne_session *sess, int serverID);
extern void syncInitial(sqlite3 *ptr, ne_session *sess, int serverID);

#endif	/*	neon_H	*/
