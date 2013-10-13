/*
 * neon_tx.c
 */

#ifndef ContactCards_H
#include "ContactCards.h"
#endif

static int getUserAuth(void *trans, const char *realm, int attempts, char *username, char *password) {
	printfunc(__func__);

	credits_t			*key = NULL;
	int					id;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;

	if (attempts > 4){
		dbgCC("[%s] must EXIT now\n", __func__);
		return 5;
	}

	id = GPOINTER_TO_INT(data->element);
	ptr = data->db;

	key = (credits_t *)calloc(1, sizeof(credits_t));

	readCardServerCredits(id, key, ptr);

	if(key->user == NULL) return 5;

	g_stpcpy(username, key->user);
	g_stpcpy(password, key->passwd);

	free(key);

	return attempts;
}

ne_session *serverConnect(int serverID, sqlite3 *ptr){
	printfunc(__func__);

	char				*davServer = NULL;
	ne_uri				uri;
	ne_session			*sess = NULL;

	davServer = getSingleChar(ptr, "cardServer", "srvUrl", 1, "serverID", serverID, "", "", "", "", "", 0);

	if(davServer== NULL) return NULL;

	ne_uri_parse(davServer, &uri);

	uri.port = uri.port ? uri.port : ne_uri_defaultport(uri.scheme);

	dbgCC("[%s] %s %s %d\n", __func__, uri.scheme, uri.host, uri.port);

	 if (ne_sock_init() != 0){
		dbgCC("[%s] failed to init socket library \n", __func__);
		return NULL;
	}

	sess = ne_session_create(uri.scheme, uri.host, uri.port);

	if(uri.port == 443){
		ne_ssl_trust_default_ca(sess);
		// what to do, if CA is not trusted by default?
	}

	return sess;
}

int vDataAccept(void *userdata, ne_request *req, const ne_status *st){
	printfunc(__func__);

	return (st->code == 200);
}


static int vDataFetch(void *trans, const char *block, size_t len){
	printfunc(__func__);

	int					contactID;
	unsigned char		*tmp = NULL;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;

	contactID = GPOINTER_TO_INT(data->element);
	ptr = data->db;

	tmp = g_memdup(g_strstrip((char *)block), len);

	updateContact(ptr, contactID, tmp);

	free(tmp);
	g_free(trans);

	return len;
}

void requestOptions(int serverID, ne_session *sess, sqlite3 *ptr){
	printfunc(__func__);

	ContactCards_stack_t        *stack;

	stack = serverRequest(DAV_REQ_OPT, serverID, 0, sess, ptr);
	responseHandle(stack, sess, ptr);
}

void requestPropfind(int serverID, ne_session *sess, sqlite3 *ptr){
	printfunc(__func__);

	ContactCards_stack_t		*stack;

	stack = serverRequest(DAV_REQ_PROP_1, serverID, 0, sess, ptr);
	responseHandle(stack, sess, ptr);

	switch(stack->statuscode){
		case 200 ... 299:
			break;
		default:
			return;
	}

	stack = serverRequest(DAV_REQ_PROP_3, serverID, 0, sess, ptr);
	responseHandle(stack, sess, ptr);

	checkAddressbooks(ptr, serverID, 10, sess);
}

const char *cookieSet(const char *srvTx){
	printfunc(__func__);

	char			*cookie;

	g_strdelimit((gchar *)srvTx, ";", '\n');
	g_strstrip((gchar *)srvTx);

	cookie = g_strndup(srvTx, strlen(srvTx));

	return cookie;
}

static int elementStart(void *userdata, int parent, const char *nspace, const char *name, const char **atts){
	printfunc(__func__);

	int					i = 0;
	ContactCards_stack_t		*stack = userdata;
	ContactCards_node_t		*ctx = NULL;
	char				*ctxName = NULL;

	while(atts[i] != NULL){
		dbgCC("\tatts[%d]: %s\n",i, nspace);
		i++;
	}

	if(parent == NE_XML_STATEROOT){
		dbgCC(" >> ROOT <<\n");
		stack->tree = g_node_new(ctx);
		stack->lastBranch = stack->tree;
	} else {
		stack->lastBranch = g_node_append_data(stack->lastBranch, g_node_new(ctx));
	}


	ctx = (ContactCards_node_t *) calloc(1, sizeof(ContactCards_node_t));
	((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data = ctx;

	ctxName = g_strdup(name);
	((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->name = ctxName;

	dbgCC("[%s]\t%s\n", __func__, ctxName);

	return 1;
}

static int elementData(void *userdata, int state, const char *cdata, size_t len){
	printfunc(__func__);

	char				*newCtx;
	char				*existingCtx;
	char				*concat;

	if(len < 7)
		return 0;

	newCtx = g_strndup(cdata, len);

	if(((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->content == NULL){
		((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->content = newCtx;
	} else {
		existingCtx = g_strdup(((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->content);
		concat = g_strconcat(existingCtx, newCtx, NULL);
		((ContactCards_node_t *)((GNode *)((ContactCards_stack_t *)userdata)->lastBranch)->data)->content = concat;
	}

	dbgCC("[%s]\t\t%s\n", __func__, newCtx);

	return 0;
}

static int elementEnd(void *userdata, int state, const char *nspace, const char *name){
	printfunc(__func__);

	ContactCards_stack_t		*stack = userdata;
	GNode				*pNode;

	pNode = stack->lastBranch;
	stack->lastBranch = pNode->parent;

	return 0;
}

int cbReader(void *userdata, const char *buf, size_t len){
	printfunc(__func__);

	ne_xml_parser *parser = (ne_xml_parser *)userdata;
	ne_xml_parse(parser, buf, len);

	return 0;
}


static int responseRead(void *data, const char *block, size_t len){
	printfunc(__func__);

	void 						*cursor = NULL;
	const char 					*name, *value;
	ne_request					*req = data;

	printf("[%s] HEADER\n", __func__);
	while ((cursor = ne_response_header_iterate(req, cursor, &name, &value))) {
		printf("%s\t\t%s\n", name, value);
	}
	printf("[%s] BODY\n", __func__);
	printf("%s\n%s\n%d\n", (char *)data, block, (int) len);

	return len;
}

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
	ContactCards_trans_t		*trans = NULL;
	ContactCards_trans_t		*reqData = NULL;

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

	dbgCC("[%s] connecting to %s with %d\n", __func__, srvUrl, method);

	isOAuth = getSingleInt(ptr, "cardServer", "isOAuth", 1, "serverID", serverID, "", "");

sendAgain:
	trans = g_new(ContactCards_trans_t, 1);
	trans->db = ptr;
	trans->element = GINT_TO_POINTER(serverID);

	ne_buffer_clear(req_buffer);
	switch(method){
		/*
		 * OPTIONS
		 */
		case DAV_REQ_OPT:
			req = ne_request_create(sess, "OPTIONS", davPath);
			ne_add_response_body_reader(req, ne_accept_always, responseRead, req);

			break;
		/*
		 * PROPFIND-Request to find the baselocation of the dav-collection
		 */
		case DAV_REQ_PROP_1:
			req = ne_request_create(sess, "PROPFIND", davPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START_EMPTY, DAV_PROP_STD, DAV_PROPFIND_END, NULL);
			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);
			break;

		/*
		 * PROPFIND-Request to find the Addressbook in all the dav-collections
		 */
		case DAV_REQ_PROP_3:
			req = ne_request_create(sess, "PROPFIND", davPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START, DAV_PROP_FIND_ADDRESSBOOK, DAV_PROPFIND_END, NULL);
			ne_add_depth_header(req, NE_DEPTH_ONE);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		/*
		 * PROPFIND-Request to get the sync-settings of the addressbook
		 *
		 *			This is a seperated request, due to the fact the addressbook had to exist in the database first
		 *
		 */
		case DAV_REQ_PROP_4:
			addrbookPath = getSingleChar(ptr, "addressbooks", "path", 14, "cardServer", serverID, "", "", "", "", "addressbookID", itemID);
			if(addrbookPath== NULL) goto failedRequest;
			req = ne_request_create(sess, "PROPFIND", addrbookPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_START, DAV_PROP_SYNC_ADDRESSBOOK, DAV_PROPFIND_END, NULL);
			ne_add_depth_header(req, NE_DEPTH_ONE);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		/*
		 * PROPFIND-Request to sync contacts from google
		 */
		case DAV_REQ_PROP_5:
			davPath = getSingleChar(ptr, "addressbooks", "path", 1, "addressbookID", itemID, "", "", "", "", "", 0);
			if(davPath== NULL) goto failedRequest;
			req = ne_request_create(sess, "PROPFIND", davPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_PROPFIND_SYNC_CONTACTS, NULL);
			ne_add_depth_header(req, NE_DEPTH_ONE);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		/*
		 * REPORT
		 */
		case DAV_REQ_REP_1:
			addrbookPath = getSingleChar(ptr, "addressbooks", "path", 14, "cardServer", serverID, "", "", "", "", "addressbookID", itemID);
			if(addrbookPath== NULL) goto failedRequest;
			req = ne_request_create(sess, "REPORT", addrbookPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_REP_SYNC_START, DAV_REP_SYNC_BASE, DAV_REP_SYNC_END, NULL);
			ne_add_depth_header(req, NE_DEPTH_ONE);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		case DAV_REQ_REP_2:
			davPath = getSingleChar(ptr, "addressbooks", "path", 1, "addressbookID", itemID, "", "", "", "", "", 0);
			if(davPath== NULL) goto failedRequest;
			davSyncToken = getSingleChar(ptr, "addressbooks", "syncToken", 1, "addressbookID", itemID, "", "", "", "", "", 0);
			if(davSyncToken== NULL) goto failedRequest;
			req = ne_request_create(sess, "REPORT", davPath);
			ne_buffer_concat(req_buffer, DAV_XML_HEAD, DAV_SYNC_TOKEN_START, davSyncToken, DAV_SYNC_TOKEN_END, NULL);
			ne_add_depth_header(req, NE_DEPTH_ONE);
			ne_add_request_header(req, "Content-Type", NE_XML_MEDIA_TYPE);

			ne_xml_push_handler(pXML, elementStart, elementData, elementEnd, userdata);
			ne_add_response_body_reader(req, ne_accept_207, cbReader, pXML);
			break;

		case DAV_REQ_GET:
			davPath = getSingleChar(ptr, "contacts", "href", 1, "contactID", itemID, "", "", "", "", "", 0);
			if(davPath == NULL) goto failedRequest;
			req = ne_request_create(sess, "GET", davPath);
			reqData = g_new(ContactCards_trans_t, 1);
			reqData->db = ptr;
			reqData->element = GINT_TO_POINTER(itemID);
			ne_add_response_body_reader(req, vDataAccept, vDataFetch, reqData);
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
			ne_add_response_body_reader(req, ne_accept_2xx, responseOAuthHandle, trans);
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
			ne_add_response_body_reader(req, ne_accept_2xx, responseOAuthHandle, trans);
			ne_add_request_header(req, "Content-Type", "application/x-www-form-urlencoded");
			}
			break;
		default:
			dbgCC("no request without method\n");
			goto failedRequest;
	}

	ContactCardsIdent = g_strconcat("ContactCards/", VERSION, NULL);
	ne_add_request_header(req, "Connection", "Keep-Alive");
	ne_set_useragent(sess, ContactCardsIdent);

	if(isOAuth) {
		char 		*authToken = NULL;
		oAuthSession = getSingleChar(ptr, "cardServer", "oAuthAccessToken", 1, "serverID", serverID, "", "", "", "", "", 0);
		authToken = g_strconcat(" Bearer ", oAuthSession, NULL);
		dbgCC("[%s] adding:Authorization %s\n", __func__, authToken);
		ne_add_request_header(req, "Authorization", authToken);
	} else {
		ne_set_server_auth(sess, getUserAuth, trans);
	}

	if(davCookie != NULL){
		dbgCC("[%s] add Cookie %s to header\n", __func__, davCookie);
		ne_add_request_header(req, "Cookie", davCookie);
		ne_add_request_header(req, "Cookie2", "$Version=1");
	} else {
		dbgCC("[%s] cookie not set\n", __func__);
	}

	ne_set_request_body_buffer(req, req_buffer->data, ne_buffer_size(req_buffer));
	ne_set_read_timeout(sess, 30);
	ne_set_connect_timeout(sess, 30);

	((ContactCards_stack_t *)userdata)->reqMethod = method;
	((ContactCards_stack_t *)userdata)->serverID = serverID;
	((ContactCards_stack_t *)userdata)->addressbookID = itemID;

	if (ne_request_dispatch(req)) {
		dbgCC("[%s] Request failed - %s\n", __func__, ne_get_error(sess));
		if(failed++ > 3) goto failedRequest;
		goto sendAgain;
	}

	statuscode = ne_get_status(req)->code;
	((ContactCards_stack_t *)userdata)->statuscode = statuscode;

	switch(statuscode){
		case 100 ... 199:
			dbgCC("==\t1xx Informational\t==\n");
			break;
		case 207:
			dbgCC("==\t207\t==\n");
			break;
		case 200 ... 206:
		case 208 ... 299:
			dbgCC("==\t2xx Success\t==\n");
			break;
		case 301:
			dbgCC("==\t301 Moved Permanently\t==\n");
			dbgCC("%s\n", ne_get_response_header(req, "Location"));
			updateUri(ptr, serverID, g_strdup(ne_get_response_header(req, "Location")), TRUE);
			if(failed++ > 3) goto failedRequest;
			goto sendAgain;
			break;
		case 300:
		case 302 ... 399:
			dbgCC("==\t3xx Redirection\t==\n");
			break;
		case 401:
			dbgCC("==\t401\t==\n");
			dbgCC("Unauthorized\n");
			if(failed++ > 3) goto failedRequest;
			goto sendAgain;
			break;
		case 405:
			dbgCC("==\t405\t==\n");
			dbgCC("Method not Allowed\n");
			break;
		case 400:
		case 402 ... 404:
		case 406 ... 499:
			dbgCC("==\t4xx Client Error\t==\n");
			dbgCC("\t\t%d\n", statuscode);
			if(failed++ > 3) goto failedRequest;

			const char *srvTx = NULL;
			srvTx = ne_get_response_header(req, "Set-Cookie");

			if(srvTx != NULL) {
				dbgCC("[%s] Set-Cookie\t%s\n", __func__, srvTx);
			} else {
				srvTx = ne_get_response_header(req, "Set-Cookie2");
				if(srvTx != NULL){
					dbgCC("[%s] Set-Cookie2\t%s\n", __func__, srvTx);
				} else {
					goto sendAgain;
				}
			}
			if(srvTx){
				davCookie =  cookieSet(srvTx);
				dbgCC("[%s] => %s\n", __func__, davCookie);
			}

			dbgCC("[%s] %s\n", __func__, ne_get_error(sess));

			goto sendAgain;
			break;
		case 500 ... 599:
			dbgCC("==\t5xx Server Error\t==\n");
			dbgCC("[%s] %s\n", __func__, ne_get_error(sess));
			break;
		default:
			dbgCC("[%s] Can't handle %d\n", __func__, statuscode);
	}

failedRequest:
	ne_request_destroy(req);
	ne_buffer_destroy(req_buffer);
	ne_xml_destroy(pXML);
	g_free(trans);

	return userdata;
}

void serverDisconnect(ne_session *sess, sqlite3 *ptr){
	printfunc(__func__);

	int			isOAuth = 0;

	if(isOAuth){
		setSingleChar(ptr, "cardServer", "oAuthAccessToken", NULL, "", 0);
	}

	ne_close_connection(sess);
	ne_forget_auth(sess);
	ne_session_destroy(sess);
}

void oAuthAccess(sqlite3 *ptr, int serverID, int oAuthServerEntity, int type){
	printfunc(__func__);

	char				*srvURI = NULL;
	char				*grant = NULL;
	ne_uri				uri;
	ne_session			*sess = NULL;

	grant = getSingleChar(ptr, "cardServer", "oAuthAccessGrant", 1, "serverID", serverID, "", "", "", "", "", 0);

	if(strlen(grant) < 5){
		dbgCC("[%s] there is no oAuthAccessGrant\n", __func__);
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
		dbgCC("[%s] failed to init socket library \n", __func__);
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

	ne_close_connection(sess);
	ne_session_destroy(sess);
}
