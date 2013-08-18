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
#define DAV_ADDRBOOK_MULTIGET			2
#define DAV_ADDRBOOK_QUERY				4

/*
 * Request-Methods
 */
#define	DAV_REQ_OPT						111
#define	DAV_REQ_PROP_1					121
#define	DAV_REQ_PROP_2					122
#define	DAV_REQ_PROP_3					123
#define DAV_REQ_PROP_4					124
#define DAV_REQ_PROP_5					125
#define DAV_REQ_REP_1					131
#define DAV_REQ_REP_2					132
#define	DAV_REQ_LOCK					141
#define	DAV_REQ_UNLOCK					151
#define	DAV_REQ_GET						161
#define DAV_REQ_GET_GRANT				171
#define	DAV_REQ_GET_TOKEN				172
#define DAV_REQ_GET_REFRESH				173

/*
 * OAuth-Elements
 */
#define OAUTH_ACCESS_GRANT				200
#define	OAUTH_ACCESS_TOKEN				201
#define	OAUTH_REFRESH_TOKEN				202
#define	OAUTH_TOKEN_TYPE				203
#define	OAUTH_EXPIRES_IN				204

/*
 * general Stuff
 */
#define BUFFERSIZE						8192
#define DAV_XML_HEAD					"<?xml version=\"1.0\" encoding=\"utf-8\" ?>"

/*
 * PROPFIND
 */
#define DAV_PROPFIND_START				"<propfind xmlns=\"DAV:\" xmlns:C=\"urn:ietf:params:xml:ns:carddav\">"
#define DAV_PROPFIND_START_EMPTY		"<propfind xmlns=\"DAV:\">"
#define DAV_PROPFIND_END				"</propfind>"

#define DAV_PROP_STD					"<prop><current-user-principal/></prop>"
#define DAV_PROP_ADDRESSBOOK_HOME		"<prop><C:addressbook-home-set/></prop>"
#define	DAV_PROP_ADDRESSBOOK_SETS		"<prop><supported-report-set/></prop>"
#define DAV_PROP_FIND_ADDRESSBOOK		"<prop><displayname/><resourcetype/></prop>"
#define DAV_PROP_SYNC_ADDRESSBOOK		"<prop><displayname/><resourcetype/><supported-report-set/></prop>"
#define DAV_PROPFIND_SYNC_CONTACTS		"<propfind xmlns=\"DAV:\"><prop><getetag /></prop></propfind>"


/*
 * REPORT
 */
#define DAV_REP_SYNC_START				"<sync-collection xmlns=\"DAV:\">"
#define DAV_REP_SYNC_BASE				"<sync-token/><prop><getetag/></prop>"
#define DAV_REP_SYNC_END				"</sync-collection>"
#define DAV_SYNC_TOKEN_START			"<D:sync-collection xmlns:D=\"DAV:\"><D:sync-token>"
#define DAV_SYNC_TOKEN_END     			"</D:sync-token><D:prop><D:getetag/></D:prop></D:sync-collection>"

#define ContactCards_NO_LEAVE					512
#define ContactCards_ELEMENT_END				513

/*
 * DAV-Elementtypes
 */
#define	DAV_ELE_HREF					801
#define DAV_ELE_DISPLAYNAME				802
#define DAV_ELE_STATUS_200				803
#define	DAV_ELE_ADDRESSBOOK				804
#define DAV_ELE_ETAG					805
#define DAV_ELE_SYNCTOKEN				806
#define DAV_ELE_ADDRBOOK_MULTIGET		807
#define DAV_ELE_ADDRBOOK_QUERY			808
#define DAV_ELE_SYNC_COLLECTION			809

typedef struct ContactCards_stack {
	GNode			*tree;
	GNode			*lastBranch;
	int				serverID;
	int				addressbookID;
	int				statuscode;
	int				reqMethod;
}ContactCards_stack_t;

typedef struct ContactCards_node {
	char			*name;
	char			*content;
} ContactCards_node_t;

typedef struct ContactCards_leaf {
	char			*status;
	char			*contentType;
	char			*href;
	char			*etag;
} ContactCards_leaf_t;

typedef struct credits{
	char			*user;
	char			*passwd;
} credits_t;


extern ne_session *serverConnect(int serverID, sqlite3 *ptr);
extern ContactCards_stack_t *serverRequest(int method, int serverID, int itemID, ne_session *sess, sqlite3 *ptr);
extern void serverDisconnect(ne_session *sess, sqlite3 *ptr);
extern void responseHandle(ContactCards_stack_t *stack, ne_session *sess, sqlite3 *ptr);
extern void requestPropfind(int serverID, ne_session *sess, sqlite3 *ptr);
extern void oAuthAccess(sqlite3 *ptr, int serverID, int oAuthServerEntity, int type);
extern int responseOAuthHandle(void *data, const char *block, size_t len);

#endif	/*	neon_H	*/
