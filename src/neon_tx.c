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
 * getUserAuth - returns the user credentials for a server
 */
static int getUserAuth(void *trans, const char *realm, int attempts, char *username, char *password) {
	printfunc(__func__);

	credits_t			key;
	int					id;

    memset(&key, 0, sizeof(key));

	if (attempts > 4){
		verboseCC("[%s] must EXIT now\n", __func__);
		return 5;
	}

	id = GPOINTER_TO_INT(trans);

	readCardServerCredits(id, &key, appBase.db);

	if(key.user == NULL) return 5;

	g_stpcpy(username, key.user);
	g_stpcpy(password, key.passwd);

	return attempts;
}

/**
 * validateUrl -checks if url is valid
 */
gboolean validateUrl(char *url){
	printfunc(__func__);

	ne_uri				uri;
	ne_sock_addr		*addr;

	if(ne_uri_parse(url, &uri) != 0)
		return FALSE;

	if(uri.host == NULL){
		ne_uri_free(&uri);
		return FALSE;
	}

	addr = ne_addr_resolve(uri.host, 0);

	if (ne_addr_result(addr)){
		ne_addr_destroy(addr);
		ne_uri_free(&uri);
		return FALSE;
	}

	ne_uri_free(&uri);
	ne_addr_destroy(addr);

	return TRUE;
}

/**
 * verifyCert - verify a certificate of a server
 */
static int verifyCert(void *trans, int failures, const ne_ssl_certificate *cert){
	printfunc(__func__);

	char						*digest = calloc(1, NE_SSL_DIGESTLEN);
	int							trust = ContactCards_DIGEST_UNTRUSTED;
	int							exists = 0;
	int							serverID;
	char						*newCer = NULL;
	char						*dbDigest = NULL;
	char						*issued = NULL;
	char						*issuer = NULL;

	serverID = GPOINTER_TO_INT(trans);

	exists = countElements(appBase.db, "certs", 1, "serverID", serverID, "", "", "", "");

	if(exists == 0){
		goto newCert;
	}

	if (failures & NE_SSL_NOTYETVALID)
		verboseCC("[%s] certificate is not yet valid\n", __func__);

	if (failures & NE_SSL_EXPIRED)
		verboseCC("[%s] certificate has expired\n", __func__);

	if (failures & NE_SSL_IDMISMATCH)
		verboseCC("[%s] hostname does not match the hostname of the server\n", __func__);

	if (failures & NE_SSL_UNTRUSTED)
		verboseCC("[%s] authority which signed the certificate is not trusted\n", __func__);

	if (failures & NE_SSL_BADCHAIN)
		verboseCC("[%s] certificate chain contained a certificate other than the server cert\n", __func__);

	if (failures & NE_SSL_REVOKED)
		verboseCC("[%s] certificate has been revoked\n", __func__);

	trust = getSingleInt(appBase.db, "certs", "trustFlag", 1, "serverID", serverID, "", "", "", "");

	switch(trust){
		case ContactCards_DIGEST_TRUSTED:
			ne_ssl_cert_digest(cert, digest);
			dbDigest = getSingleChar(appBase.db, "certs", "digest", 1, "serverID", serverID, "", "", "", "", "", 0);
			if(dbDigest == NULL){
				goto newCert;
			}
			if(g_strcmp0(digest, dbDigest) == 0){
				goto fastExit;
			}
			break;
		case ContactCards_DIGEST_UNTRUSTED:
			return ContactCards_DIGEST_UNTRUSTED;
			break;
		default:
		case ContactCards_DIGEST_NEW:
			return ContactCards_DIGEST_NEW;
			break;
	}

newCert:
	newCer = ne_ssl_cert_export(cert);
	ne_ssl_cert_digest(cert, digest);
	trust = ContactCards_DIGEST_NEW;
	issued = (char *) ne_ssl_cert_identity(cert);
	issuer = ne_ssl_readable_dname(ne_ssl_cert_issuer(cert));
	setServerCert(appBase.db, serverID, exists, trust, newCer, digest, issued, issuer);

fastExit:
	free(dbDigest);

	return trust;
}

/**
 * serverConnectionTest - tests the connection to a server
 */
int serverConnectionTest(int serverID){
	printfunc(__func__);

	char				*davServer = NULL;
	ne_uri				uri;
	ne_session			*sess = NULL;
	int					caps = 0;
	int					ret = NE_FAILED;
	int					failed = 0;

	davServer = getSingleChar(appBase.db, "cardServer", "srvUrl", 1, "serverID", serverID, "", "", "", "", "", 0);

	if(davServer== NULL) return ret;

	ne_uri_parse(davServer, &uri);
	free(davServer);
	uri.port = uri.port ? uri.port : ne_uri_defaultport(uri.scheme);

	 if (ne_sock_init() != 0){
		verboseCC("[%s] failed to init socket library \n", __func__);
		return ret;
	}

tryAgain:
	sess = ne_session_create(uri.scheme, uri.host, uri.port);

	ne_ssl_set_verify(sess, verifyCert, GINT_TO_POINTER(serverID));

	ret = ne_options2(sess, uri.path, &caps);

	switch(ret){
		case NE_OK:
			verboseCC("[%s] caps: %d\n", __func__, caps);
			break;
		case NE_ERROR:
			if(failed++ > 2){
				serverDisconnect(sess, appBase.db, serverID);
				return ret;
			}
			serverDisconnect(sess, appBase.db, serverID);
			goto tryAgain;
			break;
		default:
			verboseCC("[%s] ret: %d\tcaps: %d\n", __func__, ret, caps);
			verboseCC("[%s] %s\n", __func__, ne_get_error(sess));
	}

	serverDisconnect(sess, appBase.db, serverID);

	return ret;
}

/**
 * serverConnect - connect to a server
 */
ne_session *serverConnect(int serverID){
	printfunc(__func__);

	char				*davServer = NULL;
	ne_uri				uri;
	ne_session			*sess = NULL;

	davServer = getSingleChar(appBase.db, "cardServer", "srvUrl", 1, "serverID", serverID, "", "", "", "", "", 0);

	if(davServer== NULL) return NULL;

	ne_uri_parse(davServer, &uri);

	uri.port = uri.port ? uri.port : ne_uri_defaultport(uri.scheme);

	verboseCC("[%s] %s %s %d\n", __func__, uri.scheme, uri.host, uri.port);

	 if (ne_sock_init() != 0){
		verboseCC("[%s] failed to init socket library \n", __func__);
		return NULL;
	}

	sess = ne_session_create(uri.scheme, uri.host, uri.port);

	ne_ssl_set_verify(sess, verifyCert, GINT_TO_POINTER(serverID));

	free(davServer);

	return sess;
}

/**
 * cookieSet - set a cookie to a session connected to a server
 */
const char *cookieSet(const char *srvTx){
	printfunc(__func__);

	char			*cookie;

	g_strdelimit((gchar *)srvTx, ";", '\n');
	g_strstrip((gchar *)srvTx);

	cookie = g_strndup(srvTx, strlen(srvTx));

	return cookie;
}

/**
 * elementStart - start of an response element
 */
static int elementStart(void *userdata, int parent, const char *nspace, const char *name, const char **atts){
	printfunc(__func__);

	int					i = 0;
	ContactCards_stack_t		*stack = userdata;
	ContactCards_node_t		*ctx = NULL;
	char				*ctxName = NULL;

	while(atts[i] != NULL){
		verboseCC("\tatts[%d]: %s\n",i, nspace);
		i++;
	}

	if(parent == NE_XML_STATEROOT){
		verboseCC(" >> ROOT <<\n");
		stack->tree = g_node_new(ctx);
		stack->lastBranch = stack->tree;
	} else {
		stack->lastBranch = g_node_append_data(stack->lastBranch, g_node_new(ctx));
	}


	ctx = (ContactCards_node_t *) calloc(1, sizeof(ContactCards_node_t));
	((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data = ctx;

	ctxName = g_strdup(name);
	((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->name = ctxName;

	verboseCC("[%s]\t%s\n", __func__, ctxName);

	return 1;
}

/**
 * elementData - data of a reponse element
 */
static int elementData(void *userdata, int state, const char *cdata, size_t len){
	printfunc(__func__);

	char				*newCtx = NULL;
	char				*existingCtx = NULL;
	char				*concat = NULL;

	if(len < 3)
		return 0;

	newCtx = g_strndup(cdata, len);

	if(((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->content == NULL){
		((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->content = newCtx;
	} else {
		existingCtx = g_strdup(((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->content);
		concat = g_strconcat(existingCtx, newCtx, NULL);
		((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->content = concat;
		free(existingCtx);
		free(newCtx);
	}

	verboseCC("[%s]\t\t%s\n", __func__, newCtx);

	return 0;
}

/**
 * elementEnd - end of a response element
 */
static int elementEnd(void *userdata, int state, const char *nspace, const char *name){
	printfunc(__func__);

	ContactCards_stack_t		*stack = userdata;
	GNode				*pNode;

	pNode = stack->lastBranch;
	stack->lastBranch = pNode->parent;

	return 0;
}

/**
 * cbReader - callback function for the xml parser
 */
int cbReader(void *userdata, const char *buf, size_t len){
	printfunc(__func__);

	ne_xml_parser *parser = (ne_xml_parser *)userdata;
	ne_xml_parse(parser, buf, len);

	return 0;
}

/**
 * serverRequest - sends a request to a server
 *
 *	@method			method on how to request the server
 *	@serverID		ID for the server
 *	@itemID			ID for items. For example addressbookID or contactID
 *	@sess			session to the server you are connected to
 *	@ptr			pointer to the database
 *
 *	This function forms and sends the requests to the server
 */
ContactCards_stack_t *serverRequest(int method, int serverID, int itemID, ne_session *sess, sqlite3 *ptr){
	printfunc(__func__);

	ne_request			*req = NULL;
	ne_buffer			*req_buffer = ne_buffer_create();
	int 				statuscode = 0;
	int					failed = 0;
	int					isOAuth = 0;
	void				*userdata = calloc(1, sizeof(ContactCards_stack_t));
	ne_xml_parser		*pXML = ne_xml_create();
	char				*davPath = NULL;
	char				*srvUrl = NULL;
	char				*davSyncToken = NULL;
	char				*oAuthSession = NULL;
	const char			*davCookie = NULL;
	char				*addrbookPath = NULL;
	gchar				*ContactCardsIdent = NULL;
	ne_uri				uri;
	int					ret = 0;
	int 				count = 0;
	char				buf[BUFFERSIZE];
	gchar				*vCard = NULL;
	gchar				*tmpCard = NULL;

	switch(method){
		case DAV_REQ_GET_GRANT:
			srvUrl = getSingleChar(ptr, "oAuthServer", "grantURI", 1, "oAuthID", itemID, "", "", "", "", "", 0);
			break;
		case DAV_REQ_GET_TOKEN:
		case DAV_REQ_GET_REFRESH:
			srvUrl = getSingleChar(ptr, "oAuthServer", "tokenURI", 1, "oAuthID", itemID, "", "", "", "", "", 0);
			break;
		default:
			srvUrl = getSingleChar(ptr, "cardServer", "srvUrl", 1, "serverID", serverID, "", "", "", "", "", 0);
	}

	if(srvUrl== NULL) goto failedRequest;
	ne_uri_parse(srvUrl, &uri);
	davPath = g_strndup(uri.path, strlen(uri.path));
	ne_uri_free(&uri);

	verboseCC("[%s] connecting to %s with %d\n", __func__, srvUrl, method);

	isOAuth = getSingleInt(ptr, "cardServer", "isOAuth", 1, "serverID", serverID, "", "", "", "");

sendAgain:

	ne_buffer_clear(req_buffer);
	switch(method){
		/*
		 * Initial requests
		 */
		case DAV_REQ_EMPTY:
			req = ne_request_create(sess, "PROPFIND", davPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START, DAV_PROPFIND_END, NULL);
			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);
			break;

		case DAV_REQ_CUR_PRINCIPAL:
			req = ne_request_create(sess, "PROPFIND", davPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START,DAV_PROP_START, DAV_CUR_PRINCIPAL, DAV_PROP_END, DAV_PROPFIND_END, NULL);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		case DAV_REQ_ADDRBOOK_HOME:
			req = ne_request_create(sess, "PROPFIND", davPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START,DAV_PROP_START, DAV_ADDRBOOK_HOME, DAV_PROP_END, DAV_PROPFIND_END, NULL);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		case DAV_REQ_ADDRBOOKS:
			req = ne_request_create(sess, "PROPFIND", davPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START,DAV_PROP_START, DAV_ADDRBOOKS, DAV_PROP_END, DAV_PROPFIND_END, NULL);
			ne_add_depth_header(req, NE_DEPTH_ONE);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		case DAV_REQ_ADDRBOOK_SYNC:
			addrbookPath = getSingleChar(ptr, "addressbooks", "path", 14, "cardServer", serverID, "", "", "", "", "addressbookID", itemID);
			if(addrbookPath== NULL) goto failedRequest;
			req = ne_request_create(sess, "PROPFIND", addrbookPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START,DAV_PROP_START, DAV_ADDRBOOK_SYNC, DAV_PROP_END, DAV_PROPFIND_END, NULL);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		case DAV_REQ_REP_1:
			addrbookPath = getSingleChar(ptr, "addressbooks", "path", 14, "cardServer", serverID, "", "", "", "", "addressbookID", itemID);
			if(addrbookPath== NULL) goto failedRequest;
			req = ne_request_create(sess, "REPORT", addrbookPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_REP_SYNC_START, DAV_REP_SYNC_TOKEN, DAV_PROP_START, DAV_REP_ETAG, DAV_PROP_END, DAV_REP_SYNC_END, NULL);
			ne_add_depth_header(req, NE_DEPTH_ONE);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		case DAV_REQ_REP_2:
			addrbookPath = getSingleChar(ptr, "addressbooks", "path", 14, "cardServer", serverID, "", "", "", "", "addressbookID", itemID);
			if(addrbookPath== NULL) goto failedRequest;
			davSyncToken = getSingleChar(ptr, "addressbooks", "syncToken", 1, "addressbookID", itemID, "", "", "", "", "", 0);
			if(davSyncToken== NULL) goto failedRequest;
			req = ne_request_create(sess, "REPORT", addrbookPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_REP_SYNC_START, DAV_REP_SYNC_TOKEN_START, davSyncToken, DAV_REP_SYNC_TOKEN_END,DAV_PROP_START, DAV_REP_ETAG, DAV_PROP_END, DAV_REP_SYNC_END, NULL);
			ne_add_depth_header(req, NE_DEPTH_ONE);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		case DAV_REQ_REP_3:
			addrbookPath = getSingleChar(ptr, "addressbooks", "path", 14, "cardServer", serverID, "", "", "", "", "addressbookID", itemID);
			if(addrbookPath== NULL) goto failedRequest;
			req = ne_request_create(sess, "PROPFIND", addrbookPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START, DAV_PROP_START, DAV_REP_ETAG, DAV_PROP_END, DAV_PROPFIND_END, NULL);
			ne_add_depth_header(req, NE_DEPTH_ONE);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		/*
		 * request to get the vCard
		 */
		case DAV_REQ_GET:
			davPath = getSingleChar(ptr, "contacts", "href", 1, "contactID", itemID, "", "", "", "", "", 0);
			if(davPath == NULL) goto failedRequest;
			req = ne_request_create(sess, "GET", davPath);
			break;

		/*
		 * request to delete one contact
		 */
		case DAV_REQ_DEL_CONTACT:
			davPath = getSingleChar(ptr, "contacts", "href", 1, "contactID", itemID, "", "", "", "", "", 0);
			if(davPath == NULL) goto failedRequest;
			req = ne_request_create(sess, "DELETE", davPath);
			break;

		/*
		 * request to push a new contact to the server
		 */
		case DAV_REQ_PUT_NEW_CONTACT:
			ne_add_request_header(req, "If-None-Match", "*");
		case DAV_REQ_PUT_CONTACT:
			{
			char			*vCard = NULL;
			davPath = getSingleChar(ptr, "contacts", "href", 1, "contactID", itemID, "", "", "", "", "", 0);
			if(davPath == NULL) goto failedRequest;
			vCard = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", itemID, "", "", "", "", "", 0);
			if(vCard == NULL) goto failedRequest;
			req = ne_request_create(sess, "PUT", davPath);
			ne_add_request_header(req, "Content-Type", "text/vcard");
			ne_buffer_concat(req_buffer, vCard, NULL);
			}
			break;

		case DAV_REQ_POST_URI:
			addrbookPath = getSingleChar(ptr, "addressbooks", "path", 14, "cardServer", serverID, "", "", "", "", "addressbookID", itemID);
			if(addrbookPath== NULL) goto failedRequest;
			req = ne_request_create(sess, "PROPFIND", addrbookPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START,DAV_PROP_START, DAV_POST_URI, DAV_PROP_END, DAV_PROPFIND_END, NULL);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);
			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		case DAV_REQ_POST_CONTACT:
			{
			char			*vCard = NULL;
			int				abID = 0;
			abID = getSingleInt(ptr, "contacts", "addressbookID", 1, "contactID", itemID, "", "", "", "");
			if(abID == 0) goto failedRequest;
			davPath = getSingleChar(ptr, "addressbooks", "postURI", 1, "addressbookID", abID, "", "", "", "", "", 0);
			if(davPath == NULL) goto failedRequest;
			vCard = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", itemID, "", "", "", "", "", 0);
			if(vCard == NULL) goto failedRequest;
			req = ne_request_create(sess, "POST", davPath);
			ne_add_request_header(req, "Content-Type", "text/vcard");
			ne_buffer_concat(req_buffer, vCard, NULL);
			}
			break;

		/*
		 * oAuth-Stuff
		 */
		case DAV_REQ_GET_TOKEN:
			{
			char 	*oAuthGrant = getSingleChar(ptr, "cardServer", "oAuthAccessGrant", 1, "serverID", serverID, "", "", "", "", "", 0);
			char	*clientID = getSingleChar(ptr, "oAuthServer", "clientID", 1, "oAuthID", itemID, "", "", "", "", "", 0);
			char	*clientSec = getSingleChar(ptr, "oAuthServer", "clientSecret", 1, "oAuthID", itemID, "", "", "", "", "", 0);
			char	*redirURI = getSingleChar(ptr, "oAuthServer", "redirURI", 1, "oAuthID", itemID, "", "", "", "", "", 0);
			req = ne_request_create(sess, "POST", davPath);
			if(strlen(oAuthGrant) == 1) goto failedRequest;
			ne_buffer_concat(req_buffer,
							"code=", oAuthGrant,"&",
							"redirect_uri=", redirURI, "&",
							"client_id=", clientID, "&",
							"client_secret=", clientSec, "&",
							"grant_type=authorization_code", NULL);
			ne_add_response_body_reader(req, ne_accept_2xx, responseOAuthHandle, GINT_TO_POINTER(serverID));
			ne_add_request_header(req, "Content-Type", "application/x-www-form-urlencoded");
			}
			break;

		case DAV_REQ_GET_REFRESH:
			{
			char 	*refreshToken = getSingleChar(ptr, "cardServer", "oAuthRefreshToken", 1, "serverID", serverID, "", "", "", "", "", 0);
			char	*clientID = getSingleChar(ptr, "oAuthServer", "clientID", 1, "oAuthID", itemID, "", "", "", "", "", 0);
			char	*clientSec = getSingleChar(ptr, "oAuthServer", "clientSecret", 1, "oAuthID", itemID, "", "", "", "", "", 0);
			req = ne_request_create(sess, "POST", davPath);
			if(strlen(refreshToken) == 1) goto failedRequest;
			ne_buffer_concat(req_buffer,
							"refresh_token=", refreshToken, "&",
							"client_id=", clientID, "&",
							"client_secret=", clientSec, "&",
							"grant_type=refresh_token", NULL);
			ne_add_response_body_reader(req, ne_accept_2xx, responseOAuthHandle,  GINT_TO_POINTER(serverID));
			ne_add_request_header(req, "Content-Type", "application/x-www-form-urlencoded");
			}
			break;

		default:
			verboseCC("no request without method\n");
			verboseCC("method: %d\n", method);
			goto failedRequest;
	}

	ContactCardsIdent = g_strconcat("ContactCards/", VERSION, NULL);
	ne_add_request_header(req, "Connection", "Keep-Alive");
	ne_set_useragent(sess, ContactCardsIdent);

	if(isOAuth) {
		char 		*authToken = NULL;
		oAuthSession = getSingleChar(ptr, "cardServer", "oAuthAccessToken", 1, "serverID", serverID, "", "", "", "", "", 0);
		authToken = g_strconcat(" Bearer ", oAuthSession, NULL);
		verboseCC("[%s] adding:Authorization %s\n", __func__, authToken);
		ne_add_request_header(req, "Authorization", authToken);
	} else {
		ne_set_server_auth(sess, getUserAuth, GINT_TO_POINTER(serverID));
	}

	if(davCookie != NULL){
		verboseCC("[%s] add Cookie %s to header\n", __func__, davCookie);
		ne_add_request_header(req, "Cookie", davCookie);
		ne_add_request_header(req, "Cookie2", "$Version=1");
	} else {
		verboseCC("[%s] cookie not set\n", __func__);
	}

	ne_set_request_body_buffer(req, req_buffer->data, ne_buffer_size(req_buffer));
	ne_set_read_timeout(sess, 30);
	ne_set_connect_timeout(sess, 30);

	((ContactCards_stack_t *)userdata)->reqMethod = method;
	((ContactCards_stack_t *)userdata)->serverID = serverID;
	((ContactCards_stack_t *)userdata)->addressbookID = itemID;

	if(method == DAV_REQ_GET){
		ret = ne_begin_request(req);
		if(ret == NE_OK){
			do{
				memset(&buf, 0, BUFFERSIZE);
				ret = ne_read_response_block(req, buf, sizeof(buf));
				if(ret == 0) break;
				count += ret;
				if(vCard == NULL){
					tmpCard = g_strndup(buf, strlen(buf));
				} else {
					tmpCard = g_strconcat(vCard, buf, NULL);
				}
				vCard = g_strndup(tmpCard, strlen(tmpCard));
			} while(ret);
		if (ret == NE_OK)
			ret = ne_end_request(req);
		}
		updateContact(ptr, itemID, vCard);
		g_free(tmpCard);
		g_free(vCard);
	} else {
		switch(ne_request_dispatch(req)){
			case NE_OK:
				break;
			default:
				verboseCC("[%s] %s\n", __func__, ne_get_error(sess));
				if(failed++ > 3) goto failedRequest;
				goto sendAgain;
		}
	}

	statuscode = ne_get_status(req)->code;
	((ContactCards_stack_t *)userdata)->statuscode = statuscode;

	switch(statuscode){
		case 100 ... 199:
			verboseCC("==\t1xx Informational\t==\n");
			break;
		case 207:
			verboseCC("==\t207\t==\n");
			break;
		case 201:
			if(method == DAV_REQ_POST_CONTACT){
				updateContactUri(ptr, itemID, (char *)ne_get_response_header(req, "Location"));
			}
			break;
		case 204:
			if(method == DAV_REQ_PUT_CONTACT){
				setSingleChar(ptr, "contacts", "etag", (char *)ne_get_response_header(req, "ETag"), "contactID", itemID);
			}
			break;
		case 200:
		case 202 ... 203:
		case 205 ... 206:
		case 208 ... 299:
			verboseCC("==\t2xx Success\t==\n");
			break;
		case 301:
			verboseCC("==\t301 Moved Permanently\t==\n");
			verboseCC("%s\n", ne_get_response_header(req, "Location"));
			updateUri(ptr, serverID, g_strdup(ne_get_response_header(req, "Location")), TRUE);
			if(failed++ > 3) goto failedRequest;
			goto sendAgain;
			break;
		case 300:
		case 302 ... 399:
			verboseCC("==\t3xx Redirection\t==\n");
			break;
		case 401:
			verboseCC("==\t401\t==\n");
			verboseCC("Unauthorized\n");
			if(failed++ > 3) goto failedRequest;
			goto sendAgain;
			break;
		case 405:
			verboseCC("==\t405\t==\n");
			verboseCC("Method not Allowed\n");
			break;
		case 400:
		case 402 ... 404:
		case 406 ... 499:
			verboseCC("==\t4xx Client Error\t==\n");
			verboseCC("\t\t%d\n", statuscode);
			if(failed++ > 3) goto failedRequest;

			const char *srvTx = NULL;
			srvTx = ne_get_response_header(req, "Set-Cookie");

			if(srvTx != NULL) {
				verboseCC("[%s] Set-Cookie\t%s\n", __func__, srvTx);
			} else {
				srvTx = ne_get_response_header(req, "Set-Cookie2");
				if(srvTx != NULL){
					verboseCC("[%s] Set-Cookie2\t%s\n", __func__, srvTx);
				} else {
					goto sendAgain;
				}
			}
			if(srvTx){
				davCookie =  cookieSet(srvTx);
				verboseCC("[%s] => %s\n", __func__, davCookie);
			}

			verboseCC("[%s] %s\n", __func__, ne_get_error(sess));

			goto sendAgain;
			break;
		case 500 ... 599:
			verboseCC("==\t5xx Server Error\t==\n");
			verboseCC("[%s] %s\n", __func__, ne_get_error(sess));
			break;
		default:
			verboseCC("[%s] Can't handle %d\n", __func__, statuscode);
	}

failedRequest:
	ne_request_destroy(req);
	ne_buffer_destroy(req_buffer);
	ne_xml_destroy(pXML);
	free(davPath);
	free(addrbookPath);
	free(srvUrl);
	free(davSyncToken);
	free(oAuthSession);
	free(ContactCardsIdent);

	return userdata;
}

/**
 * serverDisconnect - end a session to a server
 */
void serverDisconnect(ne_session *sess, sqlite3 *ptr, int serverID){
	printfunc(__func__);

	int			isOAuth = 0;

	isOAuth = getSingleInt(ptr, "cardServer", "isOAuth", 1, "serverID", serverID, "", "", "", "");

	if(isOAuth){
		setSingleChar(ptr, "cardServer", "oAuthAccessToken", NULL, "serverID", serverID);
	}

	ne_close_connection(sess);
	ne_forget_auth(sess);
	ne_session_destroy(sess);
}

/**
 * oAuthUpdate - update the OAuth credentials of a server
 */
int oAuthUpdate(sqlite3 *ptr, int serverID){
	printfunc(__func__);

	char				*oAuthGrant = NULL;
	char				*oAuthToken = NULL;
	char				*oAuthRefresh = NULL;
	int					oAuthEntity = 0;
	int					ret = 0;

	oAuthGrant = getSingleChar(ptr, "cardServer", "oAuthAccessGrant", 1, "serverID", serverID, "", "", "", "", "", 0);
	oAuthEntity = getSingleInt(ptr, "cardServer", "oAuthType", 1, "serverID", serverID, "", "", "", "");
	verboseCC("[%s] connecting to a oAuth-Server\n", __func__);
	if(strlen(oAuthGrant) == 1){
		ret = OAUTH_GRANT_FAILURE;
		dialogRequestGrant(ptr, serverID, oAuthEntity);
	} else {
		verboseCC("[%s] there is already a grant\n", __func__);
	}
	oAuthRefresh = getSingleChar(ptr, "cardServer", "oAuthRefreshToken", 1, "serverID", serverID, "", "", "", "", "", 0);
	if(strlen(oAuthRefresh) == 1){
		verboseCC("[%s] there is no refresh_token\n", __func__);
		ret = OAUTH_REFRESHTOKEN_FAILURE;
		oAuthAccess(ptr, serverID, oAuthEntity, DAV_REQ_GET_TOKEN);
	} else {
		verboseCC("[%s] there is already a refresh_token\n", __func__);
		oAuthAccess(ptr, serverID, oAuthEntity, DAV_REQ_GET_REFRESH);
	}
	oAuthToken = getSingleChar(ptr, "cardServer", "oAuthAccessToken", 1, "serverID", serverID, "", "", "", "", "", 0);
	if(strlen(oAuthToken) == 1){
		verboseCC("[%s] there is no oAuthToken\n", __func__);
		ret = OAUTH_ACCESSTOKEN_FAILURE;
	} else {
		ret = OAUTH_UP2DATE;
	}

	free(oAuthGrant);
	free(oAuthToken);
	free(oAuthRefresh);

	return ret;
}

/**
 * oAuthAccess - handle OAuth for a server
 */
void oAuthAccess(sqlite3 *ptr, int serverID, int oAuthServerEntity, int type){
	printfunc(__func__);

	char				*srvURI = NULL;
	char				*grant = NULL;
	ne_uri				uri;
	ne_session			*sess = NULL;

	grant = getSingleChar(ptr, "cardServer", "oAuthAccessGrant", 1, "serverID", serverID, "", "", "", "", "", 0);

	if(strlen(grant) < 5){
		verboseCC("[%s] there is no oAuthAccessGrant\n", __func__);
		return;
	}

	switch(type){
		case DAV_REQ_GET_GRANT:
			srvURI = getSingleChar(ptr, "oAuthServer", "grantURI", 1, "oAuthID", oAuthServerEntity, "", "", "", "", "", 0);
			break;
		case DAV_REQ_GET_TOKEN:
			srvURI = getSingleChar(ptr, "oAuthServer", "grantURI", 1, "oAuthID", oAuthServerEntity, "", "", "", "", "", 0);
			break;
		case DAV_REQ_GET_REFRESH:
			srvURI = getSingleChar(ptr, "oAuthServer", "tokenURI", 1, "oAuthID", oAuthServerEntity, "", "", "", "", "", 0);
			break;
		default:
			return;
	}

	ne_uri_parse(srvURI, &uri);

	if (ne_sock_init() != 0){
		verboseCC("[%s] failed to init socket library \n", __func__);
		return;
	}

	sess = ne_session_create("https", uri.host, 443);
	ne_ssl_trust_default_ca(sess);

	switch(type){
		case DAV_REQ_GET_GRANT:
			serverRequest(DAV_REQ_GET_GRANT, serverID, oAuthServerEntity, sess, ptr);
			break;
		case DAV_REQ_GET_TOKEN:
			serverRequest(DAV_REQ_GET_TOKEN, serverID, oAuthServerEntity, sess, ptr);
			break;
		case DAV_REQ_GET_REFRESH:
			serverRequest(DAV_REQ_GET_REFRESH, serverID, oAuthServerEntity, sess, ptr);
			break;
	}

	free(srvURI);
	free(grant);

	ne_close_connection(sess);
	ne_session_destroy(sess);
}

/**
 * serverDelContact - delete a vCard from a server
 */
int serverDelContact(sqlite3 *ptr, ne_session *sess, int serverID, int selID){
	printfunc(__func__);

	ContactCards_stack_t		*stack;

	stack = serverRequest(DAV_REQ_DEL_CONTACT, serverID, selID, sess, ptr);

	switch(stack->statuscode){
		case 200 ... 299:
			break;
		default:
			return stack->statuscode;
	}

	dbRemoveItem(ptr, "contacts", 2, "", "", "contactID", selID);
	return stack->statuscode;
}

/**
 * postPushCard - send a new vCard using RFC 5995
 */
int postPushCard(sqlite3 *ptr, ne_session *sess, int srvID, int addrBookID, int newID, int oldID){
	printfunc(__func__);

	char					*postURI = NULL;
	ContactCards_stack_t	*stack;

	postURI = getSingleChar(ptr, "addressbooks", "postURI", 14, "cardServer", srvID, "", "", "", "", "addressbookID", addrBookID);
	if(strlen(postURI) <= 1){
		stack = serverRequest(DAV_REQ_POST_URI, srvID, addrBookID, sess, ptr);
		responseHandle(stack, sess, ptr);
		postURI = getSingleChar(ptr, "addressbooks", "postURI", 14, "cardServer", srvID, "", "", "", "", "addressbookID", addrBookID);
		if(strlen(postURI) <= 1){
			free(postURI);
			/* Without a URI, we can post to, we can't do anything so far	*/
			return -1;
		}
	}
	free(postURI);

	stack = serverRequest(DAV_REQ_POST_CONTACT, srvID, newID, sess, ptr);

	switch(stack->statuscode){
		case 201:
			verboseCC("[%s] 201\n", __func__);
			dbRemoveItem(ptr, "contacts", 2, "", "", "contactID", oldID);
		case 204:
			serverRequest(DAV_REQ_GET, srvID, newID, sess, ptr);
			return 1;
			break;
		default:
			return -1;
			break;
	}
	return -1;
}

/**
 * postPushCard - send a new vCard to a server
 */
int pushCard(sqlite3 *ptr, char *card, int addrBookID, int existing, int oldID){
	printfunc(__func__);

	ne_session	 			*sess = NULL;
	int						srvID;
	int						isOAuth = 0;
	int						newID = 0;
	int						ret = 0;
	ContactCards_stack_t	*stack;

	srvID = getSingleInt(ptr, "addressbooks", "cardServer", 1, "addressbookID", addrBookID, "", "", "", "");
	isOAuth = getSingleInt(ptr, "cardServer", "isOAuth", 1, "serverID", srvID, "", "", "", "");
	if(isOAuth){
		int 		ret = 0;
		ret = oAuthUpdate(ptr, srvID);
		if(ret != OAUTH_UP2DATE){
			g_mutex_unlock(&mutex);
			return -1;
		}
	}

	newID = newContact(ptr, addrBookID, card);

	sess = serverConnect(srvID);

	if(existing){
		stack = serverRequest(DAV_REQ_PUT_CONTACT, srvID, newID, sess, ptr);
	} else {
		stack = serverRequest(DAV_REQ_PUT_NEW_CONTACT, srvID, newID, sess, ptr);
	}
	switch(stack->statuscode){
		case 201:
			verboseCC("[%s] 201\n", __func__);
			dbRemoveItem(ptr, "contacts", 2, "", "", "contactID", oldID);
		case 204:
			serverRequest(DAV_REQ_GET, srvID, newID, sess, ptr);
			ret = 1;
			break;
		case 400:
			/* Try the way RFC 5995 describes	*/
			if(postPushCard(ptr, sess, srvID, addrBookID, newID, oldID) != 1){
				ret = -1;
			} else {
				ret = 1;
			}
			break;
		default:
			verboseCC("[%s] %d\n", __func__ , stack->statuscode);
			/* server didn't accept the new contact	*/
			ret = -1;
	}

	serverDisconnect(sess, ptr, srvID);

	if(ret == -1)
		dbRemoveItem(ptr, "contacts", 2, "", "", "contactID", newID);

	return ret;
}

/**
 * syncInitial - starts a initial synchronisation of a server
 */
void syncInitial(sqlite3 *ptr, ne_session *sess, int serverID){
	printfunc(__func__);

	ContactCards_stack_t		*stack;
	int							failed = 0;

sendAgain:
	stack = serverRequest(DAV_REQ_EMPTY, serverID, 0, sess, ptr);

	switch(stack->statuscode){
		case 200 ... 299:
			break;
		case 301:
			if(failed++ < 3) goto sendAgain;
			break;
		default:
			return;
	}
	responseHandle(stack, sess, ptr);

	stack = serverRequest(DAV_REQ_CUR_PRINCIPAL, serverID, 0, sess, ptr);
	responseHandle(stack, sess, ptr);

	stack = serverRequest(DAV_REQ_ADDRBOOK_HOME, serverID, 0, sess, ptr);
	responseHandle(stack, sess, ptr);

	stack = serverRequest(DAV_REQ_ADDRBOOKS, serverID, 0, sess, ptr);
	responseHandle(stack, sess, ptr);
}

/**
 * syncContacts - get all contacts in all address books
 */
void syncContacts(sqlite3 *ptr, ne_session *sess, int serverID){
	printfunc(__func__);

	if(countElements(ptr, "addressbooks", 1, "cardServer", serverID, "", "", "", "") == 0){
		/*
		 * 	Initial request to find the base stuff
		 */
		syncInitial(ptr, sess, serverID);
		checkAddressbooks(ptr, serverID, 10, sess);
		checkAddressbooks(ptr, serverID, 20, sess);
	} else {
		checkAddressbooks(ptr, serverID, 20, sess);
	}
}
