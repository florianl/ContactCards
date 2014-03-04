/*
 * 	neon.h
 */

#ifndef	neon_H
#define neon_H

#ifndef NEON_LIB_H
#include <ne_session.h>
#include <ne_socket.h>
#include <ne_utils.h>
#include <ne_request.h>
#include <ne_auth.h>
#include <ne_xml.h>
#include <ne_xmlreq.h>
#include <ne_207.h>
#include <ne_uri.h>
#include <ne_string.h>
#include <ne_basic.h>
#include <ne_props.h>
#define NEON_LIB_H
#endif	/*	NEON_LIB_H	*/

/*
 * Sync-Methods
 */
#define	DAV_SYNC_COLLECTION				1
#define	DAV_ADDRBOOK_MULTIGET			2
#define	DAV_ADDRBOOK_QUERY				4

/*
 * Request-Methods
 */
#define	DAV_REQ_EMPTY					111
#define	DAV_REQ_CUR_PRINCIPAL			112
#define	DAV_REQ_ADDRBOOK_HOME			113
#define	DAV_REQ_ADDRBOOKS				116
#define	DAV_REQ_ADDRBOOK_SYNC			117

#define	DAV_REQ_REP_1					131
#define	DAV_REQ_REP_2					132
#define	DAV_REQ_REP_3					133

#define DAV_REQ_DEL_CONTACT				141

#define	DAV_REQ_PUT_CONTACT				151
#define	DAV_REQ_POST_CONTACT			152
#define	DAV_REQ_POST_URI				153
#define	DAV_REQ_PUT_NEW_CONTACT			154

#define	DAV_REQ_GET						161
#define	DAV_REQ_GET_GRANT				171
#define	DAV_REQ_GET_TOKEN				172
#define	DAV_REQ_GET_REFRESH				173

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

#define	ContactCards_NO_LEAVE					512
#define	ContactCards_ELEMENT_END				513

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

extern ne_session *serverConnect(void *trans);
extern ContactCards_stack_t *serverRequest(int method, int serverID, int itemID, ne_session *sess, sqlite3 *ptr);
extern void serverDisconnect(ne_session *sess, sqlite3 *ptr, int serverID);
extern void responseHandle(ContactCards_stack_t *stack, ne_session *sess, sqlite3 *ptr);
extern void oAuthAccess(sqlite3 *ptr, int serverID, int oAuthServerEntity, int type);
extern int oAuthUpdate(sqlite3 *ptr, int serverID);
extern int pushCard(sqlite3 *ptr, char *card, int addrBookID, int existing);
extern int responseOAuthHandle(void *data, const char *block, size_t len);
extern int serverDelContact(sqlite3 *ptr, ne_session *sess, int serverID, int selID);
extern void syncContacts(sqlite3 *ptr, ne_session *sess, int serverID);

#endif	/*	neon_H	*/
