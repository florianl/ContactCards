/*
 * neon_rx.c
 */

#ifndef ContactCards_H
#include "ContactCards.h"
#endif

void branchDestroy(GNode *node){
	printfunc(__func__);

	while(node){
		GNode *next = node->next;
		if(node->children){
			branchDestroy(node->children);
		} else {
			g_free(((ContactCards_node_t *)node->data)->name);
			g_free(((ContactCards_node_t *)node->data)->content);
			g_node_destroy(node);
			node = next;
		}
	}
}

gboolean elementCheck(GNode *branch, int elementType){
	printfunc(__func__);

	gboolean			state = FALSE;

	while(branch){
				if(branch->children) {
					state = elementCheck(branch->children, elementType);
					if(state == TRUE)
						return TRUE;
				}

				switch(elementType){
					case DAV_ELE_STATUS_200:
							if(!strncmp(((ContactCards_node_t *)branch->data)->name, "status", 6)){
								if(!strncmp(((ContactCards_node_t *)branch->data)->content, "HTTP/1.1 200 OK", 15)) {
									return TRUE;
								} else {
									return FALSE;
								}
							}else {
								break;
							}
					case DAV_ELE_ADDRESSBOOK:
							if(!strncmp(((ContactCards_node_t *)branch->data)->name, "addressbook", 11)) {
								return TRUE;
							}else {
								break;
							}
					case DAV_ELE_ADDRBOOK_HOME:
							if(!strncmp(((ContactCards_node_t *)branch->data)->name, "addressbook-home-set", 20)) {
								return TRUE;
							}else {
								break;
							}
					case DAV_ELE_ADDRBOOK_MULTIGET:
							if(!strncmp(((ContactCards_node_t *)branch->data)->name, "addressbook-multiget", 20)) {
								return TRUE;
							}else {
								break;
							}
					case DAV_ELE_ADDRBOOK_QUERY:
							if(!strncmp(((ContactCards_node_t *)branch->data)->name, "addressbook-query", 17)) {
								return TRUE;
							}else {
								break;
							}
					case DAV_ELE_SYNC_COLLECTION:
							if(!strncmp(((ContactCards_node_t *)branch->data)->name, "sync-collection", 15)) {
								return TRUE;
							}else {
								break;
							}
					case DAV_ELE_PROXY:
							if(!strncmp(((ContactCards_node_t *)branch->data)->name, "calendar-proxy-read", 19)) {
								return TRUE;
							} else if(!strncmp(((ContactCards_node_t *)branch->data)->name, "calendar-proxy-write", 20)) {
								return TRUE;
							} else {
								break;
							}
					default:
							dbgCC("[%s] %s\n%s\n", __func__, ((ContactCards_node_t *)branch->data)->name, ((ContactCards_node_t *)branch->data)->content);
							break;
				}
				branch = branch->next;
	}

	return state;
}

char *elementGet(GNode *branch, int elementType){
	printfunc(__func__);

	char				*retElement = NULL;

	while(branch){
			if(branch->children) {
				retElement = elementGet(branch->children, elementType);
				if(retElement != NULL)
					return retElement;
			}

			switch(elementType){
				case DAV_ELE_HREF:
					if(!strncmp(((ContactCards_node_t *)branch->data)->name, "href", 4)){
						return ((ContactCards_node_t *)branch->data)->content;
					}
				case DAV_ELE_ETAG:
					if(!strncmp(((ContactCards_node_t *)branch->data)->name, "getetag", 7)){
						return ((ContactCards_node_t *)branch->data)->content;
					}
				case DAV_ELE_SYNCTOKEN:
					if(!strncmp(((ContactCards_node_t *)branch->data)->name, "sync-token", 10)){
						return ((ContactCards_node_t *)branch->data)->content;
					}
				case DAV_ELE_DISPLAYNAME:
					if (!strncmp(((ContactCards_node_t *)branch->data)->name, "displayname", 11)){
						if(((ContactCards_node_t *)branch->data)->content == NULL){
							return NULL;
						} else {
							return ((ContactCards_node_t *)branch->data)->content;
						}
					}
				default:
					dbgCC("[%s] %s\n%s\n", __func__, ((ContactCards_node_t *)branch->data)->name, ((ContactCards_node_t *)branch->data)->content);
					break;
			}
		branch = branch->next;
	}

	return retElement;
}

void locateCurUserPrincipal(GNode *tree, int serverID, sqlite3 *ptr){
	printfunc(__func__);

	if(!strncmp(((ContactCards_node_t *)tree->data)->name, "href", 4)){
		updateUri(ptr, serverID, ((ContactCards_node_t *)tree->data)->content, FALSE);
		return;
	}

	while(tree){
				if(tree->children) {
					locateCurUserPrincipal(tree->children, serverID, ptr);
				}
				tree = tree->next;
	}
}

char *getAddressbookHomeSet(GNode *branch){
	printfunc(__func__);

	char		*ret = NULL;

	if(!strncmp(((ContactCards_node_t *)branch->data)->name, "addressbook-home-set", 20)){
		ret = elementGet(branch, DAV_ELE_HREF);
		return ret;
	}

	while(branch){
				if(branch->children) {
					ret = getAddressbookHomeSet(branch->children);
				}
				branch = branch->next;
	}

	return ret;
}

void locateAddressbookHomeSet(GNode *branch, int serverID, sqlite3 *ptr){
	printfunc(__func__);

	char			*href;

	if(elementCheck(branch, DAV_ELE_STATUS_200) != TRUE){
		return;
	}

	if(elementCheck(branch, DAV_ELE_ADDRBOOK_HOME) != TRUE){
		return;
	}

	if(elementCheck(branch, DAV_ELE_PROXY) == TRUE){
		dbgCC("\tProxyelement found...\n");
		return;
	}

	href = getAddressbookHomeSet(branch);

	if(href == NULL){
		return;
	}

	updateUri(ptr, serverID, href, TRUE);
}

void locatePropstatBase(GNode *branch, int serverID, sqlite3 *ptr, int reqMethod){
	printfunc(__func__);

	if(!strncmp(((ContactCards_node_t *)branch->data)->name, "propstat", 8)){
		switch(reqMethod){
			case DAV_ELE_CUR_PRINCIPAL:
				locateCurUserPrincipal(branch, serverID, ptr);
				break;
			case DAV_ELE_ADDRBOOK_HOME:
				locateAddressbookHomeSet(branch, serverID, ptr);
				break;
			default:
				dbgCC("Can't handle %d\n", reqMethod);
		}
		return;
	}

	g_node_reverse_children(branch);

	while(branch){
		if(branch->children) {
				locatePropstatBase(branch->children, serverID, ptr, reqMethod);
		}
		branch = branch->prev;
	}

}

void locateAddressbooks(GNode *branch, int serverID, sqlite3 *ptr){

	char		*href = NULL;
	char		*displayname = NULL;

	if(elementCheck(branch, DAV_ELE_STATUS_200) != TRUE){
		return;
	}

	if(elementCheck(branch, DAV_ELE_ADDRESSBOOK) != TRUE){
		return;
	}

	href = elementGet(branch, DAV_ELE_HREF);
	displayname = elementGet(branch, DAV_ELE_DISPLAYNAME);

	newAddressbook(ptr, serverID, displayname, href);
}

void setAddrbookSync(GNode *branch, int addressbookID, sqlite3 *ptr){
	printfunc(__func__);

	int			synMethod = 0;
	int			synDB = 0;

	synDB = getSingleInt(ptr, "addressbooks", "syncMethod", 1, "addressbookID", addressbookID, "", "");
	if(synDB == -1) return;

	if(elementCheck(branch, DAV_ELE_ADDRBOOK_MULTIGET) == TRUE){
		synMethod |= DAV_ADDRBOOK_MULTIGET;
	}

	if(elementCheck(branch, DAV_ELE_ADDRBOOK_QUERY) == TRUE){
		synMethod |= DAV_ADDRBOOK_QUERY;
	}

	if(elementCheck(branch, DAV_ELE_SYNC_COLLECTION) == TRUE){
		synMethod |= DAV_SYNC_COLLECTION;
	}

	if(synDB == synMethod){
		return;
	}

	setSingleInt(ptr, "addressbooks", "syncMethod", synMethod, "addressbookID", addressbookID);
}

void reportHandle(GNode *branch, int addressbookID, int serverID, ne_session *sess, sqlite3 *ptr){
	printfunc(__func__);

	char		*href = NULL;
	char		*etag = NULL;
	char		*syncToken = NULL;
	char		*dbSyncToken = NULL;

	syncToken = elementGet(branch, DAV_ELE_SYNCTOKEN);
	if(syncToken != NULL){
		dbSyncToken = getSingleChar(ptr, "addressbooks", "syncToken", 1, "addressbookID", addressbookID, "", "", "", "", "", 0);
		if(strlen(dbSyncToken) == 1) return;
		if(!strncmp(syncToken, dbSyncToken, strlen(syncToken))) return;
		updateSyncToken(ptr, addressbookID, syncToken);
		return;
	}

	if(elementCheck(branch, DAV_ELE_STATUS_200) != TRUE){
		return;
	}

	href = elementGet(branch, DAV_ELE_HREF);
	etag =  elementGet(branch, DAV_ELE_ETAG);

	contactHandle(ptr, href, etag, serverID, addressbookID, sess);

}

void branchHandle(GNode *branch, int serverID, int addressbookID, int reqMethod, ne_session *sess, sqlite3 *ptr){
	printfunc(__func__);

	branch->parent = NULL;
	branch->next = NULL;
	branch->prev = NULL;

	switch(reqMethod){
			/*
			 *	Initial requests
			 */
			case DAV_REQ_CUR_PRINCIPAL:
				locatePropstatBase(branch, serverID, ptr, DAV_ELE_CUR_PRINCIPAL);
				break;
			case DAV_REQ_ADDRBOOK_HOME:
				locatePropstatBase(branch, serverID, ptr, DAV_ELE_ADDRBOOK_HOME);
				break;
			case DAV_REQ_ADDRBOOKS:
				locateAddressbooks(branch, serverID, ptr);
				break;
			case DAV_REQ_ADDRBOOK_SYNC:
				setAddrbookSync(branch, addressbookID, ptr);
				break;
			case DAV_REQ_EMPTY:
				return;
				break;
			case DAV_REQ_REP_1:
			case DAV_REQ_REP_2:
			case DAV_REQ_REP_3:
				reportHandle(branch, addressbookID, serverID, sess, ptr);
				break;
			default:
				dbgCC("Can't handle %d\n", reqMethod);
		}

	branchDestroy(branch);
}

void responseHandle(ContactCards_stack_t *stack, ne_session *sess, sqlite3 *ptr){
	printfunc(__func__);

	GNode				*child;

	if(stack->statuscode != 207){
		dbgCC(">> statuscode: %d <<\n", stack->statuscode);
		return;
	}

	child = stack->tree->children;

	/*
	 * split the whole response in its response-elements
	 * each response-elements will be handled in its own branchHandle()
	 */
	while(child){
		GNode	*next = child->next;
		branchHandle(child, stack->serverID, stack->addressbookID, stack->reqMethod, sess, ptr);
		child = next;
	}

	branchDestroy(child);

	return;
}

void responseElementOAuthHandle(sqlite3 *db, int serverID, char *element){
	printfunc(__func__);

	gchar			**ptr = NULL;
	int				i = 0;
	int				tokenType = 0;

	ptr = g_strsplit(element, ":", -1);

	while(ptr[i] != NULL){
		char		*item = NULL;
		char		*value = NULL;
		g_strstrip(ptr[i]);
		item = g_shell_unquote(ptr[i], NULL);
		if(g_strcmp0(item, "error") == 0){
			dbgCC("[%s] had to handle: %s - %s\n", __func__, item, ptr[++i]);
			break;
		} else if(g_strcmp0(item, "access_token") == 0){
			tokenType = OAUTH_ACCESS_TOKEN;
		} else if(g_strcmp0(item, "token_type") == 0){
			tokenType = OAUTH_TOKEN_TYPE;
		} else if(g_strcmp0(item, "refresh_token") == 0){
			tokenType = OAUTH_REFRESH_TOKEN;
		} else if(g_strcmp0(item, "expires_in") == 0){
			tokenType = OAUTH_EXPIRES_IN;
		} else {
			dbgCC("[%s] had to handle: %s\n", __func__, item);
			break;
		}
		g_free(item);
		i++;
		g_strstrip(ptr[i]);
		value = g_shell_unquote(ptr[i], NULL);
		updateOAuthCredentials(db, serverID, tokenType, value);
		g_free(value);
		i++;
	}
}

int responseOAuthHandle(void *trans, const char *block, size_t len){
	printfunc(__func__);

	gchar			**tokens = NULL;
	gchar			**ptr = NULL;
	int				items = 0;
	sqlite3 		*db;
	int 			serverID;
	ContactCards_trans_t	*data = trans;

	db = data->db;
	serverID = GPOINTER_TO_INT(data->element);

	tokens = g_strsplit(block, "\n", -1);
	ptr = tokens;
	while (*ptr){
		g_strstrip(*ptr);
		if(g_strcmp0(*ptr, "{") == 0 || g_strcmp0(*ptr, "}") == 0){
			++ptr;
			items++;
			continue;
		}
		responseElementOAuthHandle(db, serverID, *ptr);
		++ptr;
	}
	g_strfreev (tokens);

	return items;
}
