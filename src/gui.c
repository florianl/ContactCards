/*
 *	gui.c
 */

#ifndef ContactCards_H
#include "ContactCards.h"
#endif

void guiRun(sqlite3 *ptr){
	printfunc(__func__);

	fillList(ptr, 1, 0, addressbookList);
	gtk_main();
}

void guiExit(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GSList			*cleanUpList = data;

	gtk_main_quit();
	g_slist_free_full(cleanUpList, g_free);
}


void guiKeyHandler(GtkWidget *gui, GdkEventKey *event, gpointer data){
	printfunc(__func__);

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		guiExit(gui, data);
	}
}

void dialogKeyHandler(GtkDialog *widget, GdkEventKey *event, gpointer data){
	printfunc(__func__);

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		gtk_dialog_response(widget, GTK_RESPONSE_DELETE_EVENT);
	}
}

static void dialogAbout(void){
	printfunc(__func__);

	gtk_show_about_dialog(NULL,
		"title", _("About ContactCards"),
		"program-name", "ContactCards",
		"comments", _("Address book written in C using DAV"),
		"website", "http://der-flo.net",
		"license", "GNU General Public License, version 2\nhttp://www.gnu.org/licenses/old-licenses/gpl-2.0.html",
		"version", VERSION,
		NULL);
}

static void selBook(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	char				*selText;
	int					selID;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;

	ptr = data->db;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, TEXT_COLUMN, &selText, ID_COLUMN, &selID,  -1);
		printf("[%s] %d\n",__func__, selID);
		fillList(ptr, 2, selID, contactList);
		g_free(selText);
	}
}

void prefServerDelete(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers;

	ptr = data->db;
	buffers = data->element2;

	printf("[%s] %d\n", __func__, buffers->srvID);
	
	dbRemoveItem(ptr, "cardServer", 2, "", "", "serverID", buffers->srvID);
	cleanUpRequest(ptr, buffers->srvID, 0);
	fillList(ptr, 3, 0, buffers->srvPrefList);
	fillList(ptr, 1, 0, addressbookList);
}

void prefServerSave(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers;

	ptr = data->db;
	buffers = data->element2;

	updateServerDetails(ptr, buffers->srvID,
						gtk_entry_buffer_get_text(buffers->descBuf), gtk_entry_buffer_get_text(buffers->urlBuf), gtk_entry_buffer_get_text(buffers->userBuf), gtk_entry_buffer_get_text(buffers->passwdBuf));
}

void prefServerSelect(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkWidget			*prefFrame;
	int					selID;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers;
	char				*frameTitle = NULL, *user = NULL, *passwd = NULL, *url = NULL;
	int					isOAuth;

	ptr = data->db;
	prefFrame = data->element;
	buffers = data->element2;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
		printf("[%s] %d\n",__func__, selID);
		frameTitle = getSingleChar(ptr, "cardServer", "desc", 1, "serverID", selID, "", "", "", "", "", 0);
		gtk_frame_set_label(GTK_FRAME(prefFrame), frameTitle);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->descBuf), frameTitle, -1);

		buffers->srvID = selID;

		user = getSingleChar(ptr, "cardServer", "user", 1, "serverID", selID, "", "", "", "", "", 0);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->userBuf), user, -1);

		isOAuth = getSingleInt(ptr, "cardServer", "isOAuth", 1, "serverID", selID, "", "");
		if(!isOAuth){
			passwd = getSingleChar(ptr, "cardServer", "passwd", 1, "serverID", selID, "", "", "", "", "", 0);
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), passwd, -1);
		} else {
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), "", -1);
		}

		url = getSingleChar(ptr, "cardServer", "srvUrl", 1, "serverID", selID, "", "", "", "", "", 0);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->urlBuf), url, -1);
	}
}

static void selContact(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkTextBuffer		*dataBuffer;
	int					selID;
	char				*vData = NULL;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;

	ptr = data->db;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
		printf("[%s] %d\n",__func__, selID);
		vData = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", selID, "", "", "", "", "", 0);
		if(vData == NULL) return;
		dataBuffer = gtk_text_view_get_buffer(data->element);
		gtk_text_view_set_editable(data->element, FALSE);
		gtk_text_buffer_set_text(dataBuffer, vData, -1);
	}
}

void listFlush(GtkWidget *list){
	printfunc(__func__);

	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeIter  iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (list)));
	if(store == NULL) return;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
	if (gtk_tree_model_get_iter_first(model, &iter) == FALSE){
		return;
	}
	gtk_list_store_clear(store);
}

void listAppend(GtkWidget *list, gchar *text, guint id) {
	printfunc(__func__);

	GtkListStore		*store;
	GtkTreeIter 		iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(list)));

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TEXT_COLUMN, text, ID_COLUMN, id, -1);
}

void listInit(GtkWidget *list){
	printfunc(__func__);

	GtkListStore		*store;
	GtkTreeViewColumn	*column;
	GtkCellRenderer		*renderer;
	GtkTreeSortable		*sortable;


	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "text", TEXT_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_UINT);

	sortable = GTK_TREE_SORTABLE(store);

	gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));

	g_object_unref(store);
}

static void *syncOneServer(void *trans){
	printfunc(__func__);

	int					serverID = 0;
	int					isOAuth = 0;
	ne_session 			*sess = NULL;
	ContactCards_trans_t		*data = trans;
	sqlite3 			*ptr = data->db;
	GtkWidget			*statusBar;
	char				*srv = NULL;
	char				*msg = NULL;
	int					ctxID;

	g_mutex_lock(&mutex);

	serverID = GPOINTER_TO_INT(data->element);
	statusBar = data->element2;

	srv = getSingleChar(ptr, "cardServer", "desc", 1, "serverID", serverID, "", "", "", "", "", 0);
	msg = g_strconcat(_("syncing "), srv, NULL);
	ctxID = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "info");
	gtk_statusbar_push(GTK_STATUSBAR(statusBar), ctxID, msg);

	isOAuth = getSingleInt(ptr, "cardServer", "isOAuth", 1, "serverID", serverID, "", "");

	if(isOAuth){
		char				*oAuthGrant = NULL;
		char				*oAuthToken = NULL;
		char				*oAuthRefresh = NULL;
		int					oAuthEntity = 0;

		oAuthGrant = getSingleChar(ptr, "cardServer", "oAuthAccessGrant", 1, "serverID", serverID, "", "", "", "", "", 0);
		oAuthEntity = getSingleInt(ptr, "cardServer", "oAuthType", 1, "serverID", serverID, "", "");
		printf("[%s] connecting to a oAuth-Server\n", __func__);
		if(strlen(oAuthGrant) == 1){
			char		*newuser = NULL;
			newuser = getSingleChar(ptr, "cardServer", "user", 1, "serverID", serverID, "", "", "", "", "", 0);
			dialogRequestGrant(ptr, serverID, oAuthEntity, newuser);
		} else {
			printf("[%s] there is already a grant\n", __func__);
		}
		oAuthRefresh = getSingleChar(ptr, "cardServer", "oAuthRefreshToken", 1, "serverID", serverID, "", "", "", "", "", 0);
		if(strlen(oAuthRefresh) == 1){
			printf("[%s] there is no refresh_token\n", __func__);
			oAuthAccess(ptr, serverID, oAuthEntity, DAV_REQ_GET_TOKEN);
		} else {
			printf("[%s] there is already a refresh_token\n", __func__);
			oAuthAccess(ptr, serverID, oAuthEntity, DAV_REQ_GET_REFRESH);
		}
		oAuthToken = getSingleChar(ptr, "cardServer", "oAuthAccessToken", 1, "serverID", serverID, "", "", "", "", "", 0);
		if(strlen(oAuthToken) == 1){
			printf("[%s] there is no oAuthToken\n", __func__);
			g_free(data);
			g_mutex_unlock(&mutex);
			return NULL;
		}
	}

	sess = serverConnect(serverID, ptr);
	requestPropfind(serverID, sess, ptr);
	checkAddressbooks(ptr, serverID, 20, sess);
	serverDisconnect(sess, ptr);

	gtk_statusbar_pop(GTK_STATUSBAR(statusBar), ctxID);

	g_free(data);
	g_mutex_unlock(&mutex);

	return NULL;
}

static void dialogExportContacts(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	sqlite3						*ptr;
	ContactCards_trans_t		*data = trans;
	GtkWidget					*dirChooser;
	int							result;
	char						*path = NULL;

	ptr = data->db;

	dirChooser = gtk_file_chooser_dialog_new(_("Export Contacts"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	g_signal_connect(G_OBJECT(dirChooser), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	result = gtk_dialog_run(GTK_DIALOG(dirChooser));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dirChooser));
			exportContacts(ptr, path);
			g_free(path);
			break;
		default:
			break;
	}
	gtk_widget_destroy(dirChooser);
}

static void syncServer(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	sqlite3						*ptr;
	ContactCards_trans_t		*data = trans;
	GSList						*retList;
	GtkWidget					*statusBar;
	GError		 				*error = NULL;
	ContactCards_trans_t		*buff = NULL;

	ptr = data->db;
	statusBar = data->element;

	if(selectedSrv != 0){
		buff = g_new(ContactCards_trans_t, 1);
		buff->db = ptr;
		buff->element = GINT_TO_POINTER(selectedSrv);
		buff->element2 = statusBar;
		g_thread_try_new("syncingServer", syncOneServer, buff, &error);
		if(error){
			printf("[%s] something has gone wrong with threads\n", __func__);
		}
	} else {

		retList = getListInt(ptr, "cardServer", "serverID", 0, "", 0, "", "");

		while(retList){
			GSList				*next = retList->next;
			int					serverID = GPOINTER_TO_INT(retList->data);
			if(serverID == 0){
				retList = next;
				continue;
			}
			buff = g_new(ContactCards_trans_t, 1);
			buff->db = ptr;
			buff->element = GINT_TO_POINTER(serverID);
			buff->element2 = statusBar;
			g_thread_try_new("syncingServer", syncOneServer, buff, &error);
			if(error){
				printf("[%s] something has gone wrong with threads\n", __func__);
			}
			retList = next;
		}
		g_slist_free(retList);
	}
}

static void comboChanged(GtkComboBox *combo, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					id = 0;
	int 				counter = 0;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;

	ptr = data->db;

	if( gtk_combo_box_get_active_iter(combo, &iter)){
		model = gtk_combo_box_get_model(combo);
		gtk_tree_model_get( model, &iter, 1, &id, -1);
	}

	counter = countDuplicate(ptr, "cardServer", 1, "serverID", id, "", "", "", "");

	if(counter == 1) {
		fillList(ptr, 1, id, addressbookList);
		selectedSrv = id;
	} else {
		fillList(ptr, 1, 0, addressbookList);
		selectedSrv = 0;
	}
}

void comboAppend(GtkListStore *store, gchar *text, guint id) {
	printfunc(__func__);

	GtkTreeIter 		iter;

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TEXT_COLUMN, text, ID_COLUMN, id, -1);
}

void comboFlush(GtkListStore *store){
	printfunc(__func__);

	GtkTreeIter			iter;

	gtk_list_store_clear(store);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TEXT_COLUMN, "All", ID_COLUMN, 0, -1);

}

GtkWidget *comboInit(sqlite3 *ptr){
	printfunc(__func__);

	GtkWidget			*combo;
	GtkCellLayout		*layout;
	GtkCellRenderer		*renderer;
	ContactCards_trans_t		*trans = NULL;

	comboList = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_UINT);

	fillCombo(ptr, comboList);

	combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(comboList));

	layout = GTK_CELL_LAYOUT(combo);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(layout, renderer, FALSE);
	gtk_cell_layout_set_attributes(layout, renderer, "text", 0, NULL);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	g_object_unref(comboList);

	trans = g_new(ContactCards_trans_t, 1);
	trans->db = ptr;

	g_signal_connect(combo, "changed", G_CALLBACK(comboChanged), trans);

	return combo;
}

void dialogRequestGrant(sqlite3 *ptr, int serverID, int entity, char *newuser){
	printfunc(__func__);

	GtkWidget			*dialog, *area, *box, *label;
	GtkWidget			*input, *button;
	GtkEntryBuffer		*grant;
	char				*newGrant = NULL;
	int					result;
	char				*uri = NULL;

	grant = gtk_entry_buffer_new(NULL, -1);

	dialog = gtk_dialog_new_with_buttons("Request for Grant", GTK_WINDOW(mainWindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	uri = g_strdup("https://accounts.google.com/o/oauth2/auth?scope=https://www.googleapis.com/auth/carddav&redirect_uri=urn:ietf:wg:oauth:2.0:oob&response_type=code&client_id=741969998490.apps.googleusercontent.com");

	area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	button = gtk_link_button_new_with_label(uri, _("Request Grant"));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 2);
	label = gtk_label_new(_("Access Grant"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(grant);
	gtk_box_pack_start(GTK_BOX(box), input, FALSE, FALSE, 2);

	gtk_box_pack_start(GTK_BOX(area), box, FALSE, FALSE, 2);

	g_signal_connect(G_OBJECT(dialog), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			newGrant = g_strdup(gtk_entry_buffer_get_text(grant));
			g_strdelimit(newGrant, ",", '\n');
			g_strstrip(newGrant);
			if(strlen(newGrant) < 5){
				dbRemoveItem(ptr, "cardServer", 2, "", "", "serverID", serverID);
				break;
			}
			setSingleChar(ptr, "cardServer", "oAuthAccessGrant", newGrant, "serverID", serverID);
			g_free(newGrant);
			oAuthAccess(ptr, serverID, entity, DAV_REQ_GET_TOKEN);
			break;
		default:
			dbRemoveItem(ptr, "cardServer", 2, "", "", "serverID", serverID);
			break;
	}
	g_free(uri);
	gtk_widget_destroy(dialog);

}

void dialogNewServer(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget			*dialog, *notebook, *box, *box2;
	GtkWidget			*area, *label, *input;
	GtkWidget			*regFrame, *oAuthFrame;
	GtkEntryBuffer		*desc, *url, *user, *passwd;
	GtkEntryBuffer		*desc2, *user2;
	int					result;
	int					regNote, oAuthNote;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;

	ptr = data->db;

	desc = gtk_entry_buffer_new(NULL, -1);
	url = gtk_entry_buffer_new(NULL, -1);
	user = gtk_entry_buffer_new(NULL, -1);
	passwd = gtk_entry_buffer_new(NULL, -1);
	desc2 = gtk_entry_buffer_new(NULL, -1);
	user2 = gtk_entry_buffer_new(NULL, -1);

	dialog = gtk_dialog_new_with_buttons(_("New Server"), GTK_WINDOW(mainWindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	notebook = gtk_notebook_new();

	area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(area), notebook, FALSE, FALSE, 2);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	regFrame = gtk_frame_new("");
	label = gtk_label_new(_("Description"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(desc);
	gtk_box_pack_start(GTK_BOX(box), input, FALSE, FALSE, 2);

	label = gtk_label_new(_("URL"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(url);
	gtk_box_pack_start(GTK_BOX(box), input, FALSE, FALSE, 2);

	label = gtk_label_new(_("User"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(user);
	gtk_box_pack_start(GTK_BOX(box), input, FALSE, FALSE, 2);

	label = gtk_label_new(_("Password"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(passwd);
	gtk_entry_set_visibility(GTK_ENTRY(input), FALSE);
	gtk_box_pack_start(GTK_BOX(box), input, FALSE, FALSE, 2);

	gtk_container_add(GTK_CONTAINER(regFrame), box);
	label = gtk_label_new(_("Regular"));
	regNote = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), regFrame, label);

	box2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	oAuthFrame = gtk_frame_new("");
	label = gtk_label_new(_("Description"));
	gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(desc2);
	gtk_box_pack_start(GTK_BOX(box2), input, FALSE, FALSE, 2);

	label = gtk_label_new(_("User"));
	gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(user2);
	gtk_box_pack_start(GTK_BOX(box2), input, FALSE, FALSE, 2);

	gtk_container_add(GTK_CONTAINER(oAuthFrame), box2);
	label = gtk_label_new(_("oAuth2"));
	oAuthNote = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), oAuthFrame, label);

	g_signal_connect(G_OBJECT(dialog), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			if (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) == regNote){
				if(gtk_entry_buffer_get_length(desc)== 0) break;
				if(gtk_entry_buffer_get_length(url)== 0) break;
				if(gtk_entry_buffer_get_length(user)== 0) break;
				if(gtk_entry_buffer_get_length(passwd)== 0) break;
				newServer(ptr, (char *) gtk_entry_buffer_get_text(desc), (char *) gtk_entry_buffer_get_text(user), (char *) gtk_entry_buffer_get_text(passwd), (char *) gtk_entry_buffer_get_text(url));
			} else if (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) == oAuthNote) {
				if(gtk_entry_buffer_get_length(desc2)== 0) break;
				if(gtk_entry_buffer_get_length(user2)== 0) break;
				newServerOAuth(ptr, g_strdup(gtk_entry_buffer_get_text(desc2)), g_strdup(gtk_entry_buffer_get_text(user2)), 1);
			}
			fillCombo(ptr, comboList);
			break;
		default:
			break;
	}
	gtk_widget_destroy(dialog);
}

void prefExit(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	g_free(data);
}

void prefKeyHandler(GtkWidget *window, GdkEventKey *event, gpointer data){
	printfunc(__func__);

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		gtk_widget_destroy(window);
	}
}

void prefWindow(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget			*prefWindow, *prefView, *prefFrame, *prefList;
	GtkWidget			*serverPrefList;
	GtkWidget			*vbox, *hbox;
	GtkWidget			*label, *input;
	GtkWidget			*saveBtn, *deleteBtn;
	GtkEntryBuffer		*desc, *url, *user, *passwd;
	GtkTreeSelection	*serverSel;
	sqlite3				*ptr;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers = NULL;

	desc = gtk_entry_buffer_new(NULL, -1);
	url = gtk_entry_buffer_new(NULL, -1);
	user = gtk_entry_buffer_new(NULL, -1);
	passwd = gtk_entry_buffer_new(NULL, -1);

	buffers = g_new(ContactCards_pref_t, 1);

	ptr = data->db;

	serverPrefList = gtk_tree_view_new();
	listInit(serverPrefList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(serverPrefList), FALSE);

	prefWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(prefWindow), _("Preferences"));
	gtk_window_resize(GTK_WINDOW(prefWindow), 512, 312);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(prefWindow), TRUE);

	prefView = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	prefList = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(prefList), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(prefList, 128, -1);

	prefFrame = gtk_frame_new(_("Settings"));
	data->element = prefFrame;
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Description"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(desc);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("URL"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(url);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("User"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(user);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Password"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(passwd);
	gtk_entry_set_visibility(GTK_ENTRY(input), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	deleteBtn = gtk_button_new_with_label(_("Delete Server"));
	gtk_box_pack_start(GTK_BOX(hbox), deleteBtn, FALSE, FALSE, 2);
	saveBtn = gtk_button_new_with_label(_("Save changes"));
	gtk_box_pack_start(GTK_BOX(hbox), saveBtn, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 10);

	gtk_container_add(GTK_CONTAINER(prefFrame), vbox);
	gtk_container_add(GTK_CONTAINER(prefList), serverPrefList);
	fillList(ptr, 3, 0 , serverPrefList);

	buffers->descBuf = desc;
	buffers->urlBuf = url;
	buffers->userBuf = user;
	buffers->passwdBuf = passwd;
	buffers->btnDel = deleteBtn;
	buffers->btnSave = saveBtn;
	buffers->srvPrefList = serverPrefList;
	data->element2 = buffers;

	/*		Connect Signales		*/
	serverSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(serverPrefList));
	gtk_tree_selection_set_mode (serverSel, GTK_SELECTION_SINGLE); 
	g_signal_connect(serverSel, "changed", G_CALLBACK(prefServerSelect), data);
	g_signal_connect(G_OBJECT(prefWindow), "destroy", G_CALLBACK(prefExit), buffers);

	g_signal_connect(buffers->btnDel, "clicked", G_CALLBACK(prefServerDelete), data);
	g_signal_connect(buffers->btnSave, "clicked", G_CALLBACK(prefServerSave), data);

	g_signal_connect(G_OBJECT(prefWindow), "key_press_event", G_CALLBACK(prefKeyHandler), buffers);

	gtk_container_add(GTK_CONTAINER(prefView), prefList);
	gtk_container_add(GTK_CONTAINER(prefView), prefFrame);
	gtk_container_add(GTK_CONTAINER(prefWindow), prefView);
	gtk_widget_show_all(prefWindow);
}

void guiInit(sqlite3 *ptr){
	printfunc(__func__);

	GtkWidget			*mainVBox, *mainToolbar, *mainStatusbar, *mainContent;
	GtkWidget			*addressbookWindow;
	GtkWidget			*contactBox, *contactWindow, *contactView;
	GtkWidget			*serverCombo;
	GtkToolItem			*comboItem, *prefItem, *aboutItem, *sep, *newServer, *syncItem, *exportItem;
	GtkTreeSelection	*bookSel, *contactSel;
	GtkTextBuffer		*dataBuffer;
	GSList 				*cleanUpList = g_slist_alloc();
	ContactCards_trans_t		*transBook = NULL;
	ContactCards_trans_t		*transContact = NULL;
	ContactCards_trans_t		*transPref = NULL;
	ContactCards_trans_t		*transNew = NULL;
	ContactCards_trans_t		*transSync = NULL;

	gtk_init(NULL, NULL);

	mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(mainWindow), "ContactCards");
	gtk_window_set_default_size(GTK_WINDOW(mainWindow), 760,496);

	mainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	/*		Toolbar					*/
	mainToolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(mainToolbar), GTK_TOOLBAR_ICONS);

	newServer = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), newServer, -1);

	prefItem = gtk_tool_button_new_from_stock(GTK_STOCK_PREFERENCES);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), prefItem, -1);

	exportItem = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), exportItem, -1);

	syncItem = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), syncItem, -1);

	serverCombo = comboInit(ptr);
	comboItem = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(comboItem), serverCombo);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), comboItem, -1);

	sep = gtk_separator_tool_item_new();
	gtk_tool_item_set_expand(sep, TRUE);
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(sep), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), sep, -1);
	aboutItem = gtk_tool_button_new_from_stock(GTK_STOCK_ABOUT);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), aboutItem, -1);

	/*		Statusbar				*/
	mainStatusbar = gtk_statusbar_new();

	/*		mainContent				*/
	mainContent = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	/*		Addressbookstuff		*/
	addressbookWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(addressbookWindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(addressbookWindow, 128, -1);
	addressbookList = gtk_tree_view_new();
	listInit(addressbookList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(addressbookList), FALSE);

	/*		Contactstuff			*/
	contactBox = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	contactWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(contactWindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(contactWindow, 192, -1);
	dataBuffer = gtk_text_buffer_new(NULL);
	contactView = gtk_text_view_new_with_buffer(dataBuffer);
	gtk_container_set_border_width(GTK_CONTAINER(contactView), 10);
	contactList = gtk_tree_view_new();
	listInit(contactList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(contactList), FALSE);

	/*		preference dialog		*/
	transPref = g_new(ContactCards_trans_t, 1);
	transPref->db = ptr;
	cleanUpList = g_slist_append(cleanUpList, transPref);

	/*		new Server dialog		*/
	transNew = g_new(ContactCards_trans_t, 1);
	transNew->db = ptr;
	cleanUpList = g_slist_append(cleanUpList, transNew);

	/*		sync Server 			*/
	transSync = g_new(ContactCards_trans_t, 1);
	transSync->db = ptr;
	transSync->element = mainStatusbar;
	cleanUpList = g_slist_append(cleanUpList, transSync);

	/*		Connect Signales		*/
	transBook = g_new(ContactCards_trans_t, 1);
	transBook->db = ptr;
	cleanUpList = g_slist_append(cleanUpList, transBook);
	bookSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(addressbookList));
	gtk_tree_selection_set_mode (bookSel, GTK_SELECTION_SINGLE); 
	g_signal_connect(bookSel, "changed", G_CALLBACK(selBook), transBook);

	transContact = g_new(ContactCards_trans_t, 1);
	transContact->db = ptr;
	transContact->element = contactView;
	cleanUpList = g_slist_append(cleanUpList, transContact);
	contactSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(contactList));
	gtk_tree_selection_set_mode (contactSel, GTK_SELECTION_SINGLE); 
	g_signal_connect(contactSel, "changed", G_CALLBACK(selContact), transContact);

	g_signal_connect(G_OBJECT(mainWindow), "key_press_event", G_CALLBACK(guiKeyHandler), cleanUpList);
	g_signal_connect(G_OBJECT(mainWindow), "destroy", G_CALLBACK(guiExit), cleanUpList);
	g_signal_connect(G_OBJECT(prefItem), "clicked", G_CALLBACK(prefWindow), transPref);
	g_signal_connect(G_OBJECT(aboutItem), "clicked", G_CALLBACK(dialogAbout), NULL);
	g_signal_connect(G_OBJECT(newServer), "clicked", G_CALLBACK(dialogNewServer), transNew);
	g_signal_connect(G_OBJECT(syncItem), "clicked", G_CALLBACK(syncServer), transSync);
	g_signal_connect(G_OBJECT(exportItem), "clicked", G_CALLBACK(dialogExportContacts), transPref);

	/*		Put it all together		*/
	gtk_box_pack_start(GTK_BOX(mainVBox), mainToolbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(addressbookWindow), addressbookList);
	gtk_container_add(GTK_CONTAINER(mainContent), addressbookWindow);
	gtk_container_add(GTK_CONTAINER(contactWindow), contactList);
	gtk_container_add(GTK_CONTAINER(contactBox), contactWindow);
	gtk_container_add(GTK_CONTAINER(contactBox), contactView);
	gtk_container_add(GTK_CONTAINER(mainContent), contactBox);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainContent, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainStatusbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(mainWindow), mainVBox);
	gtk_widget_show_all(mainWindow);
}
