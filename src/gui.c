/*
 *	gui.c
 */

#include "ContactCards.h"

/**
 * guiRun - run the basic GUI
 */
void guiRun(sqlite3 *ptr){
	printfunc(__func__);

	fillList(ptr, 1, 0, addressbookList);
	fillList(ptr, 2, 0, contactList);
	gtk_main();
}

/**
 * guiExit - exit the basic GUI and clean up
 */
void guiExit(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	gtk_main_quit();
	g_slist_free_full(data, g_free);
}

/**
 * guiKeyHandler - control some kind of the basic GUI by keyboard
 */
void guiKeyHandler(GtkWidget *gui, GdkEventKey *event, gpointer data){
	printfunc(__func__);

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		guiExit(gui, data);
	}
}
/**
 * dialogKeyHandler - control some kind of a dialog by keyboard
 */
void dialogKeyHandler(GtkDialog *widget, GdkEventKey *event, gpointer data){
	printfunc(__func__);

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		gtk_dialog_response(widget, GTK_RESPONSE_DELETE_EVENT);
	}
}

/**
 * dialogAbout - display some basic stuff about this software
 */
static void dialogAbout(void){
	printfunc(__func__);

	gtk_show_about_dialog(NULL,
		"title", _("About ContactCards"),
		"program-name", "ContactCards",
		"comments", _("Address book written in C using DAV"),
		"website", "https://www.der-flo.net/ContactCards.html",
		"license", "GNU General Public License, version 2\nhttp://www.gnu.org/licenses/old-licenses/gpl-2.0.html",
		"version", VERSION,
		NULL);
}

/**
 * selBook - select a single address book
 */
static void selBook(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	char				*selText;
	int					selID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, TEXT_COLUMN, &selText, ID_COLUMN, &selID,  -1);
		dbgCC("[%s] %d\n",__func__, selID);
		fillList(((ContactCards_trans_t *) trans)->db, 2, selID, contactList);
		g_free(selText);
	}
}

/**
 * prefServerDelete - delete a server in the preferences dialog
 */
void prefServerDelete(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers;

	buffers = data->element2;

	dbgCC("[%s] %d\n", __func__, buffers->srvID);

	dbRemoveItem(data->db, "cardServer", 2, "", "", "serverID", buffers->srvID);
	dbRemoveItem(data->db, "certs", 2, "", "", "serverID", buffers->srvID);
	cleanUpRequest(data->db, buffers->srvID, 0);
	fillList(data->db, 3, 0, buffers->srvPrefList);
	fillList(data->db, 1, 0, addressbookList);

	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->descBuf), "", -1);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->urlBuf), "", -1);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->userBuf), "", -1);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), "", -1);

	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuedBuf), "", -1);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuerBuf), "", -1);
	gtk_switch_set_active(GTK_SWITCH(buffers->certSel), FALSE);
}

/**
 * prefServerSave - save changes to a server in the preferences dialog
 */
void prefServerSave(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers;

	buffers = data->element2;

	updateServerDetails(data->db, buffers->srvID,
						gtk_entry_buffer_get_text(buffers->descBuf), gtk_entry_buffer_get_text(buffers->urlBuf), gtk_entry_buffer_get_text(buffers->userBuf), gtk_entry_buffer_get_text(buffers->passwdBuf),
						gtk_switch_get_active(GTK_SWITCH(buffers->certSel)));
}

/**
 * prefServerCheck - checking for address books
 */
void prefServerCheck(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_trans_t		*data = trans;
	ContactCards_trans_t		*checkData = NULL;
	ContactCards_pref_t			*buffers;
	int							isOAuth = 0;
	ne_session					*sess = NULL;

	buffers = data->element2;
	g_mutex_lock(&mutex);

	isOAuth = getSingleInt(data->db, "cardServer", "isOAuth", 1, "serverID", buffers->srvID, "", "");

	if(isOAuth){
		int		ret = 0;
		ret = oAuthUpdate(data->db, buffers->srvID);
		if(ret != OAUTH_UP2DATE){
			g_mutex_unlock(&mutex);
			return;
		}
	}

	checkData = g_new(ContactCards_trans_t, 1);
	checkData->db = data->db;
	checkData->element = GINT_TO_POINTER(buffers->srvID);

	sess = serverConnect(checkData);
	syncInitial(data->db, sess, buffers->srvID);
	serverDisconnect(sess, data->db, buffers->srvID);
	g_free(checkData);
	g_mutex_unlock(&mutex);
	return;
}

/**
 * prefExportCert - export the certificate of a server
 */
void prefExportCert(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_trans_t	*data = trans;
	ContactCards_pref_t		*buffers;
	GtkWidget					*dirChooser;
	int							result;
	char						*path = NULL;

	buffers = data->element2;

	dirChooser = gtk_file_chooser_dialog_new(_("Export Certificate"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Export"), GTK_RESPONSE_ACCEPT, NULL);

	g_signal_connect(G_OBJECT(dirChooser), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	result = gtk_dialog_run(GTK_DIALOG(dirChooser));

	if (result == GTK_RESPONSE_ACCEPT) {
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dirChooser));
			exportCert(data->db, path, buffers->srvID);
			g_free(path);
	}
	gtk_widget_destroy(dirChooser);
}

/**
 * prefServerSelect - select a server in the preferences dialog
 */
void prefServerSelect(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkWidget			*prefFrame;
	int					selID;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers;
	char				*frameTitle = NULL, *user = NULL, *passwd = NULL;
	char				*issued = NULL, *issuer = NULL, *url = NULL;
	int					isOAuth;
	gboolean			res = 0;

	prefFrame = data->element;
	buffers = data->element2;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
		dbgCC("[%s] %d\n",__func__, selID);
		frameTitle = getSingleChar(data->db, "cardServer", "desc", 1, "serverID", selID, "", "", "", "", "", 0);
		if (frameTitle == NULL) return;
		gtk_frame_set_label(GTK_FRAME(prefFrame), frameTitle);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->descBuf), frameTitle, -1);

		buffers->srvID = selID;

		user = getSingleChar(data->db, "cardServer", "user", 1, "serverID", selID, "", "", "", "", "", 0);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->userBuf), user, -1);

		isOAuth = getSingleInt(data->db, "cardServer", "isOAuth", 1, "serverID", selID, "", "");
		if(!isOAuth){
			passwd = getSingleChar(data->db, "cardServer", "passwd", 1, "serverID", selID, "", "", "", "", "", 0);
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), passwd, -1);
		} else {
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), "", -1);
		}

		url = getSingleChar(data->db, "cardServer", "srvUrl", 1, "serverID", selID, "", "", "", "", "", 0);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->urlBuf), url, -1);

		gtk_switch_set_active(GTK_SWITCH(buffers->certSel), FALSE);

		if(countElements(data->db, "certs", 1, "serverID", selID, "", "", "", "") == 1){

			issued = getSingleChar(data->db, "certs", "issued", 1, "serverID", selID, "", "", "", "", "", 0);
			if(issued == NULL) issued = "";
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuedBuf), issued, -1);

			issuer = getSingleChar(data->db, "certs", "issuer", 1, "serverID", selID, "", "", "", "", "", 0);
			if(issuer == NULL) issuer = "";
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuerBuf), issuer, -1);

			res = getSingleInt(data->db, "certs", "trustFlag", 1, "serverID", selID, "", "");
			if(res == ContactCards_DIGEST_TRUSTED){
				gtk_switch_set_active(GTK_SWITCH(buffers->certSel), TRUE);
			} else {
				gtk_switch_set_active(GTK_SWITCH(buffers->certSel), FALSE);
			}
		} else {
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuedBuf), "", -1);
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuerBuf), "", -1);
			gtk_switch_set_active(GTK_SWITCH(buffers->certSel), FALSE);
		}
	}
	
	free(frameTitle);
	free(user);
	free(passwd);
	free(issued);
	free(issuer);
	free(url);
}

/**
 * feedbackDialog
 *	@type			type of the dialog
 *	@msg			message which will be shown
 *
 *	This function gives the user a feedback via a dialog
 */
void feedbackDialog(int type, char *msg){
	printfunc(__func__);

	GtkWidget		*infoDia;

	infoDia = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_OK, msg);
	gtk_dialog_run(GTK_DIALOG(infoDia));
	gtk_widget_destroy(infoDia);
}

/**
 * contactDel - delete a selected vCard
 */
static void contactDel(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkListStore		*store;
	GtkWidget			*dialog;
	int					selID, addrID, srvID;
	ne_session 			*sess = NULL;
	ContactCards_trans_t		*data = trans;
	ContactCards_trans_t		*delData = NULL;
	gint 				resp;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(data->element), &model, &iter)) {
		int				isOAuth = 0;
		gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
		dbgCC("[%s] %d\n",__func__, selID);

		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Do you really want to delete this contact?"));

		resp = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if (resp != GTK_RESPONSE_YES) return;

		g_mutex_lock(&mutex);
		addrID = getSingleInt(data->db, "contacts", "addressbookID", 1, "contactID", selID, "", "");
		srvID = getSingleInt(data->db, "addressbooks", "cardServer", 1, "addressbookID", addrID, "", "");

		delData = g_new(ContactCards_trans_t, 1);
		delData->db = data->db;
		delData->element = GINT_TO_POINTER(srvID);

		isOAuth = getSingleInt(data->db, "cardServer", "isOAuth", 1, "serverID", srvID, "", "");

		if(isOAuth){
			int 		ret = 0;
			ret = oAuthUpdate(data->db, srvID);
			if(ret != OAUTH_UP2DATE)
				goto failure;
		}

		sess = serverConnect(delData);
		serverDelContact(data->db, sess, srvID, selID);
		serverDisconnect(sess, data->db, srvID);

		store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(contactList)));
		gtk_list_store_remove(store, &iter);

failure:
		g_free(delData);
		g_mutex_unlock(&mutex);
	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no contact selected."));
	}
}
/**
 * contactEditPostalItem - Add a postal address to edit a vCard
 */
static int contactEditPostalItem(GtkWidget *grid, GSList *list, int line, char *value){
	printfunc(__func__);

	gchar				**postalPtr = NULL;
	GtkWidget				*input, *label;
	GtkEntryBuffer			*boxBuf, *extBuf, *streetBuf, *cityBuf, *regBuf, *zipBuf, *countryBuf;
	GSList				*elements;
	ContactCards_item_t		*boxItem, *extItem, *streetItem, *cityItem, *regItem, *zipItem, *countryItem, *eleList;

	boxItem = g_new(ContactCards_item_t, 1);
	extItem = g_new(ContactCards_item_t, 1);
	streetItem = g_new(ContactCards_item_t, 1);
	cityItem = g_new(ContactCards_item_t, 1);
	regItem = g_new(ContactCards_item_t, 1);
	zipItem = g_new(ContactCards_item_t, 1);
	countryItem = g_new(ContactCards_item_t, 1);
	eleList = g_new(ContactCards_item_t, 1);

	boxBuf = gtk_entry_buffer_new(NULL, -1);
	extBuf = gtk_entry_buffer_new(NULL, -1);
	streetBuf = gtk_entry_buffer_new(NULL, -1);
	cityBuf = gtk_entry_buffer_new(NULL, -1);
	regBuf = gtk_entry_buffer_new(NULL, -1);
	zipBuf = gtk_entry_buffer_new(NULL, -1);
	countryBuf = gtk_entry_buffer_new(NULL, -1);

	elements = g_slist_alloc();

	postalPtr = g_strsplit(value, ";", 8);
	gtk_entry_buffer_set_text(boxBuf, g_strstrip(postalPtr[0]), -1);
	gtk_entry_buffer_set_text(extBuf, g_strstrip(postalPtr[1]), -1);
	gtk_entry_buffer_set_text(streetBuf, g_strstrip(postalPtr[2]), -1);
	gtk_entry_buffer_set_text(cityBuf, g_strstrip(postalPtr[3]), -1);
	gtk_entry_buffer_set_text(regBuf, g_strstrip(postalPtr[4]), -1);
	gtk_entry_buffer_set_text(zipBuf, g_strstrip(postalPtr[5]), -1);
	gtk_entry_buffer_set_text(countryBuf, g_strstrip(postalPtr[6]), -1);

	label = gtk_label_new(_("post office box"));
	input = gtk_entry_new_with_buffer(boxBuf);
	gtk_grid_attach(GTK_GRID(grid), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 1, line++, 2, 1);
	boxItem->itemID = CARDTYPE_ADR_OFFICE_BOX;
	boxItem->element = boxBuf;
	elements = g_slist_append(elements, boxItem);

	label = gtk_label_new(_("extended address"));
	input = gtk_entry_new_with_buffer(extBuf);
	gtk_grid_attach(GTK_GRID(grid), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 1, line++, 2, 1);
	extItem->itemID = CARDTYPE_ADR_EXT_ADDR;
	extItem->element = extBuf;
	elements = g_slist_append(elements, extItem);

	label = gtk_label_new(_("street"));
	input = gtk_entry_new_with_buffer(streetBuf);
	gtk_grid_attach(GTK_GRID(grid), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 1, line++, 2, 1);
	streetItem->itemID = CARDTYPE_ADR_STREET;
	streetItem->element = streetBuf;
	elements = g_slist_append(elements, streetItem);

	label = gtk_label_new(_("city"));
	input = gtk_entry_new_with_buffer(cityBuf);
	gtk_grid_attach(GTK_GRID(grid), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 1, line++, 2, 1);
	cityItem->itemID = CARDTYPE_ADR_CITY;
	cityItem->element = cityBuf;
	elements = g_slist_append(elements, cityItem);

	label = gtk_label_new(_("region"));
	input = gtk_entry_new_with_buffer(regBuf);
	gtk_grid_attach(GTK_GRID(grid), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 1, line++, 2, 1);
	regItem->itemID = CARDTYPE_ADR_REGION;
	regItem->element = regBuf;
	elements = g_slist_append(elements, regItem);

	label = gtk_label_new(_("zip"));
	input = gtk_entry_new_with_buffer(zipBuf);
	gtk_grid_attach(GTK_GRID(grid), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 1, line++, 2, 1);
	zipItem->itemID = CARDTYPE_ADR_ZIP;
	zipItem->element = zipBuf;
	elements = g_slist_append(elements, zipItem);

	label = gtk_label_new(_("country"));
	input = gtk_entry_new_with_buffer(countryBuf);
	gtk_grid_attach(GTK_GRID(grid), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 1, line++, 2, 1);
	countryItem->itemID = CARDTYPE_ADR_COUNTRY;
	countryItem->element = countryBuf;
	elements = g_slist_append(elements, countryItem);

	eleList->itemID = CARDTYPE_ADR;
	eleList->element = elements;
	list = g_slist_append(list, eleList);

	return line;
}

/**
 * contactNewSingleItem - Add a single line value to edit a vCard
 * Here a single linked list is added to a single linked list, to
 * implement TYP-stuff in the future
 */
static void contactNewSingleItem(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget				*input, *label;
	GtkEntryBuffer			*buf;
	GSList					*elements;
	ContactCards_item_t		*item, *eleList;

	item = g_new(ContactCards_item_t, 1);
	eleList = g_new(ContactCards_item_t, 1);
	buf = gtk_entry_buffer_new(NULL, -1);

	elements = g_slist_alloc();

	switch(((ContactCards_new_Value_t *)trans)->type){
		case CARDTYPE_TEL:
			label = gtk_label_new(_("Phone"));
			break;
		case CARDTYPE_EMAIL:
			label = gtk_label_new(_("EMail"));
			break;
		case CARDTYPE_URL:
			label = gtk_label_new(_("URL"));
			break;
		case CARDTYPE_IMPP:
			label = gtk_label_new(_("IM"));
			break;
		default:
			label = gtk_label_new(NULL);
			break;
	}
	gtk_widget_show_all(label);
	gtk_grid_attach_next_to(GTK_GRID(((ContactCards_new_Value_t *)trans)->grid), label, NULL, GTK_POS_BOTTOM, 1, 1);

	input = gtk_entry_new_with_buffer(buf);
	gtk_grid_attach_next_to(GTK_GRID(((ContactCards_new_Value_t *)trans)->grid), input, label, GTK_POS_RIGHT, 2, 1);
	item->itemID = ((ContactCards_new_Value_t *)trans)->type;
	item->element = buf;
	elements = g_slist_append(elements, item);

	gtk_widget_show_all(GTK_WIDGET(input));

	eleList->itemID = ((ContactCards_new_Value_t *)trans)->type;
	eleList->element = elements;
	((ContactCards_new_Value_t *)trans)->list = g_slist_append(((ContactCards_new_Value_t *)trans)->list, eleList);

	return;
}

/**
 * contactNewSingleMultilineItem - Add a single multiline value to edit a vCard
 * Here a single linked list is added to a single linked list, to
 * implement TYP-stuff in the future
 */
static void contactNewSingleMultilineItem(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget				*input, *label;
	GtkTextBuffer			*buf;
	GSList					*elements;
	ContactCards_item_t		*item, *eleList;

	item = g_new(ContactCards_item_t, 1);
	eleList = g_new(ContactCards_item_t, 1);
	buf = gtk_text_buffer_new(NULL);

	elements = g_slist_alloc();

	switch(((ContactCards_new_Value_t *)trans)->type){
		case CARDTYPE_NOTE:
			label = gtk_label_new(_("Note"));
			break;
		default:
			label = gtk_label_new(NULL);
			break;
	}
	gtk_widget_show_all(label);
	gtk_grid_attach_next_to(GTK_GRID(((ContactCards_new_Value_t *)trans)->grid), label, NULL, GTK_POS_BOTTOM, 1, 1);

	input = gtk_text_view_new_with_buffer(buf);
	gtk_grid_attach_next_to(GTK_GRID(((ContactCards_new_Value_t *)trans)->grid), input, label, GTK_POS_RIGHT, 2, 1);
	item->itemID = ((ContactCards_new_Value_t *)trans)->type;
	item->element = buf;
	elements = g_slist_append(elements, item);

	gtk_widget_show_all(GTK_WIDGET(input));

	eleList->itemID = ((ContactCards_new_Value_t *)trans)->type;
	eleList->element = elements;
	((ContactCards_new_Value_t *)trans)->list = g_slist_append(((ContactCards_new_Value_t *)trans)->list, eleList);

	return;
}

/**
 * contactEditSingleItem - Add a single line value to edit a vCard
 * Here a single linked list is added to a single linked list, to
 * implement TYP-stuff in the future
 */
static int contactEditSingleItem(GtkWidget *grid, GSList *list, int type, int line, char *value){
	printfunc(__func__);

	GtkWidget				*input;
	GtkEntryBuffer			*buf;
	GSList					*elements;
	ContactCards_item_t		*item, *eleList;

	item = g_new(ContactCards_item_t, 1);
	eleList = g_new(ContactCards_item_t, 1);
	buf = gtk_entry_buffer_new(NULL, -1);

	elements = g_slist_alloc();

	gtk_entry_buffer_set_text(buf, g_strstrip(value), -1);

	input = gtk_entry_new_with_buffer(buf);
	gtk_grid_attach(GTK_GRID(grid), input, 1, line++, 2, 1);
	item->itemID = type;
	item->element = buf;
	elements = g_slist_append(elements, item);

	eleList->itemID = type;
	eleList->element = elements;
	list = g_slist_append(list, eleList);

	return line;
}

/**
 * contactEditSingleMultilineItem - Add a single line value to edit a vCard
 * Here a single linked list is added to a single linked list, to
 * implement TYP-stuff in the future
 */
static int contactEditSingleMultilineItem(GtkWidget *grid, GSList *list, int type, int line, char *value){
	printfunc(__func__);

	GtkWidget				*input;
	GtkTextBuffer			*buf;
	GSList					*elements;
	ContactCards_item_t		*item, *eleList;

	item = g_new(ContactCards_item_t, 1);
	eleList = g_new(ContactCards_item_t, 1);
	buf = gtk_text_buffer_new(NULL);

	elements = g_slist_alloc();

	gtk_text_buffer_set_text(buf, g_strcompress(value), -1);

	input = gtk_text_view_new_with_buffer(buf);
	gtk_grid_attach(GTK_GRID(grid), input, 1, line++, 2, 1);
	item->itemID = type;
	item->element = buf;
	elements = g_slist_append(elements, item);

	eleList->itemID = type;
	eleList->element = elements;
	list = g_slist_append(list, eleList);

	return line;
}

/**
 * cleanCard - clean up the area to display a new vCard
 */
static void cleanCard(GtkWidget *widget){
	printfunc(__func__);

	GList				*children, *child;

	children = gtk_container_get_children(GTK_CONTAINER(widget));
		for(child = children; child != NULL; child = g_list_next(child))
			gtk_widget_destroy(GTK_WIDGET(child->data));
		g_list_free(children);
}

/**
 * buildNewCard - display the data of a selected vCard
 */
static GtkWidget *buildNewCard(sqlite3 *ptr, int selID){
	printfunc(__func__);

	GtkWidget			*card, *label, *sep;
	GtkWidget			*photo, *fn, *bday;
	GSList				*list;
	GError		 		*error = NULL;
	int					line = 4;
	char				*vData = NULL;
	char				*markup;
	ContactCards_pix_t	*tmp = NULL;

	card = gtk_grid_new();
	vData = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", selID, "", "", "", "", "", 0);
	if(vData == NULL)
		return card;

	gtk_widget_set_hexpand(GTK_WIDGET(card), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(card), TRUE);
	gtk_widget_set_halign(GTK_WIDGET(card), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(card), GTK_ALIGN_START);

	/*	PHOTO	*/
	tmp = getCardPhoto(vData);
	if(tmp->size == 0){
		photo = gtk_image_new_from_icon_name("avatar-default-symbolic",   GTK_ICON_SIZE_DIALOG);
	} else {
		GdkPixbuf			*pixbuf = NULL;
		GInputStream		*ginput = g_memory_input_stream_new_from_data(tmp->pixel, tmp->size, NULL);
		pixbuf = gdk_pixbuf_new_from_stream(ginput, NULL, &error);
		if(error){
			dbgCC("[%s] %s\n", __func__, error->message);
		}
		photo = gtk_image_new_from_pixbuf (pixbuf);
	}
	gtk_widget_set_size_request(GTK_WIDGET(photo), 104, 104);
	gtk_grid_attach(GTK_GRID(card), photo, 1,1, 1,2);
	g_free(tmp);

	/*	FN	*/
	fn = gtk_label_new(NULL);
	markup = g_markup_printf_escaped ("<span size=\"18000\"><b>%s</b></span>", getSingleCardAttribut(CARDTYPE_FN, vData));
	gtk_label_set_markup (GTK_LABEL(fn), markup);
	gtk_grid_attach_next_to(GTK_GRID(card), fn, photo, GTK_POS_RIGHT, 1, 1);

	/*	BDAY	*/
	bday = gtk_label_new(getSingleCardAttribut(CARDTYPE_BDAY, vData));
	gtk_grid_attach_next_to(GTK_GRID(card), bday, fn, GTK_POS_BOTTOM, 1, 1);
	gtk_widget_set_halign(GTK_WIDGET(bday), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(bday), GTK_ALIGN_START);

	/*	Adress	*/
	list = getMultipleCardAttribut(CARDTYPE_ADR, vData);
	if (g_slist_length(list) > 1){
		label = gtk_label_new(_("Address"));
		sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
		gtk_grid_attach(GTK_GRID(card), sep, 2, line++, 1, 1);
		while(list){
				GSList				*next = list->next;
				char				*value = list->data;
				if(value != NULL){
					label = gtk_label_new(g_strstrip(g_strdelimit(value, ";", '\n')));
					gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
					gtk_grid_attach(GTK_GRID(card), label, 2, line++, 1, 1);
				}
				list = next;
		}
	}
	g_slist_free(list);
	line++;

	/*	Phone	*/
	list = getMultipleCardAttribut(CARDTYPE_TEL, vData);
	if (g_slist_length(list) > 1){
		label = gtk_label_new(_("Phone"));
		sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
		gtk_grid_attach(GTK_GRID(card), sep, 2, line++, 1, 1);
		while(list){
				GSList				*next = list->next;
				char				*value = list->data;
				if(value != NULL){
					label = gtk_label_new(g_strstrip(value));
					gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
					gtk_grid_attach(GTK_GRID(card), label, 2, line++, 1, 1);
				}
				list = next;
		}
	}
	g_slist_free(list);
	line++;

	/*	EMAIL	*/
	list = getMultipleCardAttribut(CARDTYPE_EMAIL, vData);
	if (g_slist_length(list) > 1){
		label = gtk_label_new(_("EMail"));
		sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
		gtk_grid_attach(GTK_GRID(card), sep, 2, line++, 1, 1);
		while(list){
				GSList				*next = list->next;
				char				*value = list->data;
				if(value != NULL){
					char			*uri = NULL;
					uri = g_strconcat("mailto:", g_strcompress((g_strstrip(value))), NULL);
					label = gtk_link_button_new_with_label(uri, g_strcompress((g_strstrip(value))));
					g_free(uri);
					gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
					gtk_grid_attach(GTK_GRID(card), label, 2, line++, 1, 1);
				}
				list = next;
		}
	}
	g_slist_free(list);
	line++;

	/*	URL	*/
	list = getMultipleCardAttribut(CARDTYPE_URL, vData);
	if (g_slist_length(list) > 1){
		label = gtk_label_new(_("URL"));
		sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
		gtk_grid_attach(GTK_GRID(card), sep, 2, line++, 1, 1);
		while(list){
				GSList				*next = list->next;
				char				*value = list->data;
				if(value != NULL){
					label = gtk_link_button_new (g_strcompress((g_strstrip(value))));
					gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
					gtk_grid_attach(GTK_GRID(card), label, 2, line++, 1, 1);
				}
				list = next;
		}
	}
	g_slist_free(list);
	line++;

	/*	Note	*/
	list = getMultipleCardAttribut(CARDTYPE_NOTE, vData);
	if (g_slist_length(list) > 1){
		label = gtk_label_new(_("Note"));
		sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
		gtk_grid_attach(GTK_GRID(card), sep, 2, line++, 1, 1);
		while(list){
				GSList				*next = list->next;
				char				*value = list->data;
				if(value != NULL){
					label = gtk_label_new(g_strcompress(value));
					gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
					gtk_grid_attach(GTK_GRID(card), label, 2, line++, 1, 1);
				}
				list = next;
		}
	}
	g_slist_free(list);
	line++;

	free(vData);
	free(markup);

	return card;
}

/**
 * contactEditDiscard - discard the changes on a vCard
 */
static void contactEditDiscard(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget		*card;

	card = buildNewCard(((ContactCards_add_t *)trans)->db, ((ContactCards_add_t *)trans)->editID);
	gtk_widget_show_all(card);
	cleanCard(((ContactCards_add_t *)trans)->grid);
	gtk_container_add(GTK_CONTAINER(((ContactCards_add_t *)trans)->grid), card);

	g_slist_free_full(((ContactCards_add_t *)trans)->list, g_free);
	g_free(trans);
}

/**
 * pushingCard - Threadfunction to push a vCard
 */
static void *pushingCard(void *trans){
	printfunc(__func__);

	ContactCards_trans_t		*data = trans;
	ContactCards_add_t			*value = data->element;
	char						*vCard = NULL;
	char						*dbCard = NULL;
	int							oldID = value->editID;
	int							addrID = value->aID;

	g_mutex_lock(&mutex);

	if(oldID){
		dbCard = getSingleChar(data->db, "contacts", "vCard", 1, "contactID", oldID, "", "", "", "", "", 0);
		vCard = mergeCards(value->list, dbCard);
	} else {
		vCard = buildCard(value->list);
	}

	if(pushCard(data->db, vCard, addrID, 1, oldID) == 1){
		dbRemoveItem(data->db, "contacts", 2, "", "", "contactID", oldID);
	} else {
		feedbackDialog(GTK_MESSAGE_ERROR, _("Unable to save changes"));
	}

	g_slist_free_full(value->list, g_free);
	g_free(trans);
	g_free(vCard);
	g_free(dbCard);

	g_mutex_unlock(&mutex);
	return NULL;
}

/**
 * contactEditSave - save the changes on a vCard
 */
static void contactEditSave(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_trans_t		*buff = NULL;
	GError						*error = NULL;
	GThread						*thread;

	cleanCard(((ContactCards_add_t *)trans)->grid);

	buff = g_new(ContactCards_trans_t, 1);
	buff->db = ((ContactCards_add_t *)trans)->db;
	buff->element = (ContactCards_add_t *)trans;

	thread = g_thread_try_new("pushing vCard", pushingCard, buff, &error);
	if(error){
		dbgCC("[%s] something has gone wrong with threads\n", __func__);
		dbgCC("%s\n", error->message);
	}
	g_thread_unref(thread);

}

/**
 * buildEditCard - display the data of a selected vCard for editing
 */
static GtkWidget *buildEditCard(sqlite3 *ptr, int selID, int abID){
	printfunc(__func__);

	GtkWidget			*card, *label, *sep, *input;
	GtkWidget			*addPhone, *addMail, *addUrl, *addPostal, *addIM, *addNote;
	GSList				*list, *items;
	GtkWidget			*discardBtn, *saveBtn, *row;
	GtkEntryBuffer		*prefixBuf, *firstNBuf, *middleNBuf, *lastNBuf, *suffixBuf;
	int								line = 1;
	char							*vData = NULL;
	char							*naming = NULL;
	gchar							**namingPtr = NULL;
	ContactCards_add_t				*transNew = NULL;
	ContactCards_new_Value_t		*transPhone = NULL,
									*transUrl = NULL,
									*transEMail = NULL,
									*transIM = NULL,
									*transNote = NULL;
	ContactCards_item_t				*prefixItem, *firstNItem, *middleNItem, *lastNItem, *suffixItem;

	card = gtk_grid_new();
	if(selID){
		vData = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", selID, "", "", "", "", "", 0);
		if(vData == NULL)
			return card;
	}

	transNew = g_new(ContactCards_add_t, 1);
	transPhone = g_new(ContactCards_new_Value_t, 1);
	transUrl = g_new(ContactCards_new_Value_t, 1);
	transEMail = g_new(ContactCards_new_Value_t, 1);
	transIM = g_new(ContactCards_new_Value_t, 1);
	transNote = g_new(ContactCards_new_Value_t, 1);
	items = g_slist_alloc();

	prefixItem = g_new(ContactCards_item_t, 1);
	firstNItem = g_new(ContactCards_item_t, 1);
	middleNItem = g_new(ContactCards_item_t, 1);
	lastNItem = g_new(ContactCards_item_t, 1);
	suffixItem = g_new(ContactCards_item_t, 1);

	prefixBuf = gtk_entry_buffer_new(NULL, -1);
	firstNBuf = gtk_entry_buffer_new(NULL, -1);
	middleNBuf = gtk_entry_buffer_new(NULL, -1);
	lastNBuf = gtk_entry_buffer_new(NULL, -1);
	suffixBuf = gtk_entry_buffer_new(NULL, -1);

	if(selID){
		naming = getSingleCardAttribut(CARDTYPE_N, vData);
		namingPtr = g_strsplit(naming, ";", 5);
		gtk_entry_buffer_set_text(lastNBuf, g_strstrip(namingPtr[0]), -1);
		gtk_entry_buffer_set_text(firstNBuf, g_strstrip(namingPtr[1]), -1);
		gtk_entry_buffer_set_text(middleNBuf, g_strstrip(namingPtr[2]), -1);
		gtk_entry_buffer_set_text(prefixBuf, g_strstrip(namingPtr[3]), -1);
		gtk_entry_buffer_set_text(suffixBuf, g_strstrip(namingPtr[4]), -1);
	}

	transNew->db = ptr;
	transNew->grid = card;
	transNew->list = items;
	transNew->editID = selID;
	if(!abID)
		abID = getSingleInt(ptr, "contacts", "addressbookID", 1, "contactID", selID, "", "");
	transNew->aID = abID;

	gtk_widget_set_hexpand(GTK_WIDGET(card), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(card), TRUE);
	gtk_widget_set_halign(GTK_WIDGET(card), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(card), GTK_ALIGN_START);

	row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

	discardBtn = gtk_button_new_with_label(_("Discard"));
	saveBtn = gtk_button_new_with_label(_("Save"));
	gtk_box_pack_end(GTK_BOX(row), discardBtn, FALSE, FALSE, 1);
	gtk_box_pack_end(GTK_BOX(row), saveBtn, FALSE, FALSE, 1);
	gtk_grid_attach(GTK_GRID(card), row, 0, line++, 8, 1);

	/*	Naming	*/
	label = gtk_label_new(_("Prefix"));
	input = gtk_entry_new_with_buffer(prefixBuf);
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 1, line++, 2, 1);
	prefixItem->itemID = CARDTYPE_FN_PREFIX;
	prefixItem->element = prefixBuf;
	items = g_slist_append(items, prefixItem);

	label = gtk_label_new(_("First name"));
	input = gtk_entry_new_with_buffer(firstNBuf);
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 1, line++, 2, 1);
	firstNItem->itemID = CARDTYPE_FN_FIRST;
	firstNItem->element = firstNBuf;
	items = g_slist_append(items, firstNItem);

	label = gtk_label_new(_("Middle name"));
	input = gtk_entry_new_with_buffer(middleNBuf);
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 1, line++, 2, 1);
	middleNItem->itemID = CARDTYPE_FN_MIDDLE;
	middleNItem->element = middleNBuf;
	items = g_slist_append(items, middleNItem);

	label = gtk_label_new(_("Last name"));
	input = gtk_entry_new_with_buffer(lastNBuf);
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 1, line++, 2, 1);
	lastNItem->itemID = CARDTYPE_FN_LAST;
	lastNItem->element = lastNBuf;
	items = g_slist_append(items, lastNItem);

	label = gtk_label_new(_("Suffix"));
	input = gtk_entry_new_with_buffer(suffixBuf);
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 1, line++, 2, 1);
	suffixItem->itemID = CARDTYPE_FN_SUFFIX;
	suffixItem->element = suffixBuf;
	items = g_slist_append(items, suffixItem);

	/*	Phone	*/
	label = gtk_label_new(_("Phone"));
	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	addPhone = gtk_button_new_from_icon_name("list-add", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(addPhone), _("Add a phone"));
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), sep, 1, line, 2, 1);
	gtk_grid_attach(GTK_GRID(card), addPhone, 3, line++, 1,1);
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_TEL, vData);
		if (g_slist_length(list) > 1){
			while(list){
					GSList				*next = list->next;
					char				*value = list->data;
					if(value != NULL){
						line = contactEditSingleItem(card, items, CARDTYPE_TEL, line, g_strstrip(value));
					}
					list = next;
			}
		}
		g_slist_free(list);
	}
	transPhone->grid = card;
	transPhone->list = items;
	transPhone->type = CARDTYPE_TEL;
	line++;

	/*	Address	*/
	label = gtk_label_new(_("Address"));
	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	addPostal = gtk_button_new_from_icon_name("list-add", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(addPostal), _("Add a postal address"));
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), sep, 1, line++, 2, 1);
	//gtk_grid_attach(GTK_GRID(card), addPostal, 2, line++, 1, 1);
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_ADR, vData);
		if (g_slist_length(list) > 1){
			while(list){
					GSList				*next = list->next;
					char				*value = list->data;
					if(value != NULL){
						line = contactEditPostalItem(card, items, line, g_strstrip(value));
					}
					list = next;
			}
		}
		g_slist_free(list);
	}
	line++;

	/*	EMAIL	*/
	label = gtk_label_new(_("EMail"));
	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	addMail = gtk_button_new_from_icon_name("list-add", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(addMail), _("Add a EMail"));
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), sep, 1, line, 2, 1);
	gtk_grid_attach(GTK_GRID(card), addMail, 3, line++, 1, 1);
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_EMAIL, vData);
		if (g_slist_length(list) > 1){
			while(list){
					GSList				*next = list->next;
					char				*value = list->data;
					if(value != NULL){
						line = contactEditSingleItem(card, items, CARDTYPE_EMAIL, line, g_strstrip(value));
					}
					list = next;
			}
		}
		g_slist_free(list);
	}
	transEMail->grid = card;
	transEMail->list = items;
	transEMail->type = CARDTYPE_EMAIL;
	line++;

	/*	URL	*/
	label = gtk_label_new(_("URL"));
	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	addUrl = gtk_button_new_from_icon_name("list-add", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(addUrl), _("Add a Url"));
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), sep, 1, line, 2, 1);
	gtk_grid_attach(GTK_GRID(card), addUrl, 3, line++, 1, 1);
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_URL, vData);
		if (g_slist_length(list) > 1){
			while(list){
					GSList				*next = list->next;
					char				*value = list->data;
					if(value != NULL){
						line = contactEditSingleItem(card, items, CARDTYPE_URL, line, g_strstrip(value));
					}
					list = next;
			}
		}
		g_slist_free(list);
	}
	transUrl->grid = card;
	transUrl->list = items;
	transUrl->type = CARDTYPE_URL;
	line++;

	/*	IM	*/
	label = gtk_label_new(_("IM"));
	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	addIM = gtk_button_new_from_icon_name("list-add", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(addIM), _("Add a IM"));
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), sep, 1, line, 2, 1);
	gtk_grid_attach(GTK_GRID(card), addIM, 3, line++, 1, 1);
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_IMPP, vData);
		if (g_slist_length(list) > 1){
			while(list){
					GSList				*next = list->next;
					char				*value = list->data;
					if(value != NULL){
						line = contactEditSingleItem(card, items, CARDTYPE_IMPP, line, g_strstrip(value));
					}
					list = next;
			}
		}
		g_slist_free(list);
	}
	transIM->grid = card;
	transIM->list = items;
	transIM->type = CARDTYPE_IMPP;
	line++;

	/*	Note	*/
	label = gtk_label_new(_("Note"));
	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	addNote = gtk_button_new_from_icon_name("list-add", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(addNote), _("Add a Note"));
	gtk_grid_attach(GTK_GRID(card), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), sep, 1, line, 2, 1);
	gtk_grid_attach(GTK_GRID(card), addNote, 3, line++, 1, 1);
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_NOTE, vData);
		if (g_slist_length(list) > 1){
			while(list){
					GSList				*next = list->next;
					char				*value = list->data;
					if(value != NULL){
						line = contactEditSingleMultilineItem(card, items, CARDTYPE_NOTE, line, g_strstrip(value));
					}
					list = next;
			}
		}
		g_slist_free(list);
	}
	transNote->grid = card;
	transNote->list = items;
	transNote->type = CARDTYPE_NOTE;
	line++;

	/*		Connect Signales		*/
	g_signal_connect(G_OBJECT(saveBtn), "clicked", G_CALLBACK(contactEditSave), transNew);
	g_signal_connect(G_OBJECT(discardBtn), "clicked", G_CALLBACK(contactEditDiscard), transNew);

	g_signal_connect(G_OBJECT(addPhone), "clicked", G_CALLBACK(contactNewSingleItem), transPhone);
	g_signal_connect(G_OBJECT(addMail), "clicked", G_CALLBACK(contactNewSingleItem), transEMail);
	g_signal_connect(G_OBJECT(addUrl), "clicked", G_CALLBACK(contactNewSingleItem), transUrl);
	g_signal_connect(G_OBJECT(addIM), "clicked", G_CALLBACK(contactNewSingleItem), transIM);
	g_signal_connect(G_OBJECT(addNote), "clicked", G_CALLBACK(contactNewSingleMultilineItem), transNote);

	return card;
}

/**
 * contactNew - add a new vCard
 */
static void contactNew(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget			*newCard;
	int					abID = 0;
	GtkTreeIter			iter;
	GtkTreeModel		*model;


	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(((ContactCards_trans_t *)trans)->element), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COLUMN, &abID,  -1);
	}

	dbgCC("[%s] %d\n",__func__, abID);

	if(abID == 0){
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no address book selected."));
		return;
	}

	newCard = buildEditCard(((ContactCards_trans_t *)trans)->db, 0, abID);
	cleanCard(((ContactCards_trans_t *)trans)->element2);
	gtk_widget_show_all(newCard);
	gtk_container_add(GTK_CONTAINER(((ContactCards_trans_t *)trans)->element2), newCard);
}
/**
 * contactEdit - edit the content of a vCard
 */
static void contactEdit(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkWidget			*editCard;
	int					selID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(((ContactCards_trans_t *)trans)->element), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
		dbgCC("[%s] %d\n",__func__, selID);
	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no vCard selected to edit."));
		return;
	}

	editCard = buildEditCard(((ContactCards_trans_t *)trans)->db, selID, 0);
	cleanCard(((ContactCards_trans_t *)trans)->element2);
	gtk_widget_set_halign (editCard, GTK_ALIGN_FILL);
	gtk_widget_set_valign (editCard, GTK_ALIGN_FILL);
	gtk_widget_show_all(editCard);
	gtk_container_add(GTK_CONTAINER(((ContactCards_trans_t *)trans)->element2), editCard);

}

/**
 * completionContact - select a vCard from the searchbar
 */
static void completionContact(GtkEntryCompletion *widget, GtkTreeModel *model, GtkTreeIter *iter, gpointer trans){
	printfunc(__func__);

	GtkWidget					*card;
	int							selID;

	gtk_tree_model_get(model, iter, ID_COLUMN, &selID,  -1);
	dbgCC("[%s] %d\n",__func__, selID);
	card = buildNewCard(((ContactCards_trans_t *)trans)->db, selID);
	gtk_widget_show_all(card);
	cleanCard(((ContactCards_trans_t *)trans)->element);
	gtk_container_add(GTK_CONTAINER(((ContactCards_trans_t *)trans)->element), card);
}

/**
 * selContact - select a vCard from the list
 */
static void selContact(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkWidget			*card;
	int					selID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
		dbgCC("[%s] %d\n",__func__, selID);
		card = buildNewCard(((ContactCards_trans_t *)trans)->db, selID);
		gtk_widget_show_all(card);
		cleanCard(((ContactCards_trans_t *)trans)->element);
		gtk_container_add(GTK_CONTAINER(((ContactCards_trans_t *)trans)->element), card);
	}
}

/**
 * listFlush - remove all items from a list
 */
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

/**
 * listAppend - append a new item to a list
 */
void listAppend(GtkWidget *list, gchar *text, guint id) {
	printfunc(__func__);

	GtkListStore		*store;
	GtkTreeIter 		iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(list)));

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TEXT_COLUMN, text, ID_COLUMN, id, -1);
}

/**
 * listInit - create a new list
 */
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

/**
 * listSortorderAsc - sort a list ascending
 */
static void listSortorderAsc(void){
	printfunc(__func__);

	GtkListStore		*store;

	store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(contactList)));

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 0, GTK_SORT_ASCENDING);

}

/**
 * listSortorderDesc - sort a list descending
 */
static void listSortorderDesc(void){
	printfunc(__func__);

	GtkListStore		*store;

	store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(contactList)));

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 0, GTK_SORT_DESCENDING);

}

/**
 * syncOneServer - check one single server for new data
 */
static void *syncOneServer(void *trans){
	printfunc(__func__);

	int					serverID = 0;
	int					isOAuth = 0;
	ne_session 			*sess = NULL;
	ContactCards_trans_t		*data = trans;
	GtkWidget			*statusBar;
	char				*srv = NULL;
	char				*msg = NULL;
	int					ctxID;

	g_mutex_lock(&mutex);

	serverID = GPOINTER_TO_INT(data->element);
	statusBar = data->element2;

	srv = getSingleChar(data->db, "cardServer", "desc", 1, "serverID", serverID, "", "", "", "", "", 0);
	msg = g_strconcat(_("syncing "), srv, NULL);
	ctxID = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "info");
	gtk_statusbar_push(GTK_STATUSBAR(statusBar), ctxID, msg);

	isOAuth = getSingleInt(data->db, "cardServer", "isOAuth", 1, "serverID", serverID, "", "");

	if(isOAuth){
		int 		ret = 0;
		ret = oAuthUpdate(data->db, serverID);
		if(ret != OAUTH_UP2DATE){
			g_mutex_unlock(&mutex);
			return NULL;
		}
	}

	sess = serverConnect(data);
	syncContacts(data->db, sess, serverID);
	serverDisconnect(sess, data->db, serverID);

	gtk_statusbar_pop(GTK_STATUSBAR(statusBar), ctxID);

	g_free(data);
	free(srv);
	free(msg);
	g_mutex_unlock(&mutex);

	return NULL;
}

/**
 * dialogExportContacts - dialog to export vCards
 */
static void dialogExportContacts(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_trans_t		*data = trans;
	GtkWidget					*dirChooser;
	int							result;
	char						*path = NULL;

	dirChooser = gtk_file_chooser_dialog_new(_("Export Contacts"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);

	g_signal_connect(G_OBJECT(dirChooser), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	result = gtk_dialog_run(GTK_DIALOG(dirChooser));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dirChooser));
			exportContacts(data->db, path);
			g_free(path);
			break;
		default:
			break;
	}
	gtk_widget_destroy(dirChooser);
}

/**
 * syncServer - check all available server for new data
 */
static void syncServer(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_trans_t		*data = trans;
	GSList						*retList;
	GtkWidget					*statusBar;
	GError		 				*error = NULL;
	ContactCards_trans_t		*buff = NULL;
	GThread						*thread;

	statusBar = data->element;

	if(selectedSrv != 0){
		buff = g_new(ContactCards_trans_t, 1);
		buff->db = data->db;
		buff->element = GINT_TO_POINTER(selectedSrv);
		buff->element2 = statusBar;
		thread = g_thread_try_new("syncingServer", syncOneServer, buff, &error);
		if(error){
			dbgCC("[%s] something has gone wrong with threads\n", __func__);
			dbgCC("%s\n", error->message);
		}
		g_thread_unref(thread);
	} else {

		retList = getListInt(data->db, "cardServer", "serverID", 0, "", 0, "", "", "", "");

		while(retList){
			GSList				*next = retList->next;
			int					serverID = GPOINTER_TO_INT(retList->data);
			if(serverID == 0){
				retList = next;
				continue;
			}
			buff = g_new(ContactCards_trans_t, 1);
			buff->db = data->db;
			buff->element = GINT_TO_POINTER(serverID);
			buff->element2 = statusBar;
			thread = g_thread_try_new("syncingServer", syncOneServer, buff, &error);
			if(error){
				dbgCC("[%s] something has gone wrong with threads\n", __func__);
				dbgCC("%s\n", error->message);
			}
			g_thread_unref(thread);
			retList = next;
		}
		g_slist_free(retList);
	}
}

/**
 * comboChanged - display the new stuff which is selected by the combo
 */
static void comboChanged(GtkComboBox *combo, gpointer trans){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					id = 0;
	int 				counter = 0;
	ContactCards_trans_t		*data = trans;

	if( gtk_combo_box_get_active_iter(combo, &iter)){
		model = gtk_combo_box_get_model(combo);
		gtk_tree_model_get( model, &iter, 1, &id, -1);
	}

	counter = countElements(data->db, "cardServer", 1, "serverID", id, "", "", "", "");

	if(counter == 1) {
		fillList(data->db, 1, id, addressbookList);
		selectedSrv = id;
	} else {
		fillList(data->db, 1, 0, addressbookList);
		fillList(data->db, 2, 0, contactList);
		selectedSrv = 0;
	}
}

/**
 * comboAppend - append a new item to a combo
 */
void comboAppend(GtkListStore *store, gchar *text, guint id) {
	printfunc(__func__);

	GtkTreeIter 		iter;

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TEXT_COLUMN, text, ID_COLUMN, id, -1);
}

/**
 * comboFlush - remove all items from a combo
 */
void comboFlush(GtkListStore *store){
	printfunc(__func__);

	GtkTreeIter			iter;

	gtk_list_store_clear(store);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, TEXT_COLUMN, "All", ID_COLUMN, 0, -1);

}

/**
 * comboInit - create a new combo
 */
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

/**
 * dialogRequestGrant - ask the user for a grant in a dialog
 * 
 * This is required for OAuth
 */
void dialogRequestGrant(sqlite3 *ptr, int serverID, int entity){
	printfunc(__func__);

	GtkWidget			*dialog, *area, *box, *label;
	GtkWidget			*input, *button;
	GtkEntryBuffer		*grant;
	char				*newGrant = NULL;
	int					result;
	char				*uri = NULL;

	grant = gtk_entry_buffer_new(NULL, -1);

	dialog = gtk_dialog_new_with_buttons("Request for Grant", GTK_WINDOW(mainWindow), GTK_DIALOG_DESTROY_WITH_PARENT, _("_Save"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_CANCEL, NULL);

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
	free(newGrant);
	gtk_widget_destroy(dialog);

}

/**
 * prefExit - exit the preferences dialog
 */
void prefExit(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	g_free(data);
}

/**
 * prefKeyHandler - control some kind of the preferences dialog by keyboard
 */
void prefKeyHandler(GtkWidget *window, GdkEventKey *event, gpointer data){
	printfunc(__func__);

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		gtk_widget_destroy(window);
	}
}

/**
 * prefWindow - build the preferences dialog
 */
void prefWindow(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget			*prefWindow, *prefView, *prefFrame, *prefList;
	GtkWidget			*serverPrefList;
	GtkWidget			*vbox, *hbox;
	GtkWidget			*label, *input;
	GtkWidget			*saveBtn, *deleteBtn, *exportCertBtn, *checkBtn;
	GtkWidget			*digSwitch;
	GtkWidget			*sep;
	GtkEntryBuffer		*desc, *url, *user, *passwd;
	GtkEntryBuffer		*issued, *issuer;
	GtkTreeSelection	*serverSel;
	ContactCards_trans_t		*data = trans;
	ContactCards_pref_t		*buffers = NULL;

	desc = gtk_entry_buffer_new(NULL, -1);
	url = gtk_entry_buffer_new(NULL, -1);
	user = gtk_entry_buffer_new(NULL, -1);
	passwd = gtk_entry_buffer_new(NULL, -1);
	issued = gtk_entry_buffer_new(NULL, -1);
	issuer = gtk_entry_buffer_new(NULL, -1);

	buffers = g_new(ContactCards_pref_t, 1);

	serverPrefList = gtk_tree_view_new();
	listInit(serverPrefList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(serverPrefList), FALSE);

	prefWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(prefWindow), _("Preferences"));
	gtk_window_resize(GTK_WINDOW(prefWindow), 640, 384);
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

	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	exportCertBtn = gtk_button_new_with_label(_("Export Certificate"));
	gtk_box_pack_start(GTK_BOX(hbox), exportCertBtn, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 10);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Certificate is issued for "));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(issued);
	gtk_editable_set_editable(GTK_EDITABLE(input), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Certificate issued by "));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	input = gtk_entry_new_with_buffer(issuer);
	gtk_editable_set_editable(GTK_EDITABLE(input), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Trust Certificate?"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	digSwitch = gtk_switch_new();
	gtk_box_pack_start(GTK_BOX(hbox), digSwitch, FALSE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	checkBtn = gtk_button_new_with_label(_("Check for address books"));
	gtk_box_pack_start(GTK_BOX(hbox), checkBtn, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	deleteBtn = gtk_button_new_with_label(_("Delete Server"));
	gtk_box_pack_start(GTK_BOX(hbox), deleteBtn, FALSE, FALSE, 2);
	saveBtn = gtk_button_new_with_label(_("Save changes"));
	gtk_box_pack_start(GTK_BOX(hbox), saveBtn, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 10);

	gtk_container_add(GTK_CONTAINER(prefFrame), vbox);
	gtk_container_add(GTK_CONTAINER(prefList), serverPrefList);
	fillList(data->db, 3, 0 , serverPrefList);

	buffers->descBuf = desc;
	buffers->urlBuf = url;
	buffers->userBuf = user;
	buffers->passwdBuf = passwd;
	buffers->issuedBuf = issued;
	buffers->issuerBuf = issuer;
	buffers->srvPrefList = serverPrefList;
	buffers->certSel = digSwitch;
	data->element2 = buffers;

	/*		Connect Signales		*/
	serverSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(serverPrefList));
	gtk_tree_selection_set_mode (serverSel, GTK_SELECTION_SINGLE);
	g_signal_connect(serverSel, "changed", G_CALLBACK(prefServerSelect), data);
	g_signal_connect(G_OBJECT(prefWindow), "destroy", G_CALLBACK(prefExit), buffers);

	g_signal_connect(deleteBtn, "clicked", G_CALLBACK(prefServerDelete), data);
	g_signal_connect(saveBtn, "clicked", G_CALLBACK(prefServerSave), data);
	g_signal_connect(exportCertBtn, "clicked", G_CALLBACK(prefExportCert), data);
	g_signal_connect(checkBtn, "clicked", G_CALLBACK(prefServerCheck), data);

	g_signal_connect(G_OBJECT(prefWindow), "key_press_event", G_CALLBACK(prefKeyHandler), buffers);

	gtk_container_add(GTK_CONTAINER(prefView), prefList);
	gtk_container_add(GTK_CONTAINER(prefView), prefFrame);
	gtk_container_add(GTK_CONTAINER(prefWindow), prefView);
	gtk_widget_show_all(prefWindow);
}

/**
 * newDialogEntryChanged - check for changes of a server in the preferences dialog
 */
static void newDialogEntryChanged(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkAssistant			*assistant = GTK_ASSISTANT (data);
	GtkWidget				*box;
	int						curPage;
	GtkEntryBuffer			*buf1, *buf2;
	int						buf1l, buf2l;

	buf1 = buf2 = NULL;
	buf1l = buf2l = 0;
	curPage = gtk_assistant_get_current_page(assistant);
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant),curPage);

	switch(curPage){
		case 1:
			buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "descEntry");
			buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "urlEntry");
			break;
		case 2:
			buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "userEntry");
			buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "passwdEntry");
			break;
		case 3:
			buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "oAuthEntry");
			buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "grantEntry");
			break;
		default:
			break;
	}

	buf1l = gtk_entry_buffer_get_length(buf1);
	buf2l = gtk_entry_buffer_get_length(buf2);
	if(buf1l !=0 && buf2l !=0)
				gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), box, TRUE);
}

/**
 * newDialogClose - close the dialog for a new server
 */
static void newDialogClose(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	gtk_widget_destroy(widget);
}

/**
 * newDialogApply - apply the user credentials to a new server
 */
static void newDialogApply(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget					*assistant, *box;
	GtkWidget					*controller;
	GtkEntryBuffer				*buf1, *buf2, *buf3, *buf4;
	ContactCards_trans_t		*data = trans;

	assistant = (GtkWidget*)widget;

	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 0);
	controller = (GtkWidget*)g_object_get_data(G_OBJECT(box),"google");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controller))){
		buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "oAuthEntry");
		goto google;
	}

	controller = (GtkWidget*)g_object_get_data(G_OBJECT(box),"fruux");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controller))){
		goto fruux;
	}

	/*	others		*/
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 2);
	buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "userEntry");
	buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "passwdEntry");
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 1);
	buf3 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "descEntry");
	buf4 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "urlEntry");
	newServer(data->db, (char *) gtk_entry_buffer_get_text(buf3), (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), (char *) gtk_entry_buffer_get_text(buf4));
	return;

fruux:
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 2);
	buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "userEntry");
	buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "passwdEntry");
	newServer(data->db, "fruux", (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), "https://dav.fruux.com");
	return;

google:
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 3);
	buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "oAuthEntry");
	buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "grantEntry");
	newServerOAuth(data->db, "google", (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), 1);
	return;
}

/**
 * newDialogConfirm - finally confirm to add a new server
 */
static void newDialogConfirm(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box, *hbox;
	GtkWidget				*label;

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Confirm"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(box);
	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("Confirm"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, TRUE);
}

/**
 * newDialogOAuthCredentials - credentials for OAuth
 */
static void newDialogOAuthCredentials(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box, *hbox;
	GtkWidget				*label, *inputoAuth, *inputGrant, *button;
	GtkEntryBuffer			*user, *grant;
	char					*uri = NULL;

	user = gtk_entry_buffer_new(NULL, -1);
	grant = gtk_entry_buffer_new(NULL, -1);

	uri = g_strdup("https://accounts.google.com/o/oauth2/auth?scope=https://www.googleapis.com/auth/carddav&redirect_uri=urn:ietf:wg:oauth:2.0:oob&response_type=code&client_id=741969998490.apps.googleusercontent.com");

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("User"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputoAuth = gtk_entry_new_with_buffer(user);
	g_object_set_data(G_OBJECT(box),"oAuthEntry", user);
	gtk_box_pack_start(GTK_BOX(hbox), inputoAuth, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);


	button = gtk_link_button_new_with_label(uri, _("Request Grant"));
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Grant"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputGrant = gtk_entry_new_with_buffer(grant);
	g_object_set_data(G_OBJECT(box),"grantEntry", grant);
	gtk_box_pack_start(GTK_BOX(hbox), inputGrant, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(box);
	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("OAuth"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, FALSE);

	g_signal_connect (G_OBJECT(inputoAuth), "changed", G_CALLBACK(newDialogEntryChanged), widget);
	g_signal_connect (G_OBJECT(inputGrant), "changed", G_CALLBACK(newDialogEntryChanged), widget);

	free(uri);
}

/**
 * newDialogUserCredentials - basic credentials for a new server
 */
static void newDialogUserCredentials(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box, *hbox;
	GtkWidget				*label, *inputUser, *inputPasswd;
	GtkEntryBuffer			*user, *passwd;

	user = gtk_entry_buffer_new(NULL, -1);
	passwd = gtk_entry_buffer_new(NULL, -1);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("User"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputUser = gtk_entry_new_with_buffer(user);
	g_object_set_data(G_OBJECT(box),"userEntry", user);
	gtk_box_pack_start(GTK_BOX(hbox), inputUser, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);


	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Password"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputPasswd = gtk_entry_new_with_buffer(passwd);
	gtk_entry_set_visibility(GTK_ENTRY(inputPasswd), FALSE);
	g_object_set_data(G_OBJECT(box),"passwdEntry", passwd);
	gtk_box_pack_start(GTK_BOX(hbox), inputPasswd, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(box);
	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("Credentials"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, FALSE);

	g_signal_connect (G_OBJECT(inputUser), "changed", G_CALLBACK(newDialogEntryChanged), widget);
	g_signal_connect (G_OBJECT(inputPasswd), "changed", G_CALLBACK(newDialogEntryChanged), widget);
}

/**
 * newDialogServerSettings - ask for basic informaion of the new server
 */
static void newDialogServerSettings(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box, *hbox;
	GtkWidget				*label, *inputDesc, *inputUrl;
	GtkEntryBuffer			*desc, *url;

	desc = gtk_entry_buffer_new(NULL, -1);
	url = gtk_entry_buffer_new(NULL, -1);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("Description"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputDesc = gtk_entry_new_with_buffer(desc);
	g_object_set_data(G_OBJECT(box),"descEntry", desc);
	gtk_box_pack_start(GTK_BOX(hbox), inputDesc, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);


	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("URL"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	inputUrl = gtk_entry_new_with_buffer(url);
	g_object_set_data(G_OBJECT(box),"urlEntry", url);
	gtk_box_pack_start(GTK_BOX(hbox), inputUrl, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 2);

	gtk_widget_show_all(box);
	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("Serversettings"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, FALSE);

	g_signal_connect (G_OBJECT(inputDesc), "changed", G_CALLBACK(newDialogEntryChanged), widget);
	g_signal_connect (G_OBJECT(inputUrl), "changed", G_CALLBACK(newDialogEntryChanged), widget);
}

/**
 * newDialogSelectPage - guide the user through the new server dialog
 */
gint newDialogSelectPage(gint curPage, gpointer data){
	printfunc(__func__);

	GtkWidget			*widget;
	GtkWidget			*box;
	GtkWidget			*controller;

	widget = (GtkWidget*)data;
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(widget),curPage);

	if(curPage == 0){
		controller = (GtkWidget*)g_object_get_data(G_OBJECT(box),"google");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controller))){
				return 3;
		}
		controller = (GtkWidget*)g_object_get_data(G_OBJECT(box),"fruux");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controller))){
				return 2;
		}
	}else if (curPage == 2){
		return 4;
	}
	return curPage+1;
}

/**
 * newDialogSelectType - select the type of the new server
 */
static void newDialogSelectType(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	GtkWidget				*box;
	GtkWidget				*fruux, *google, *others;

	fruux = gtk_radio_button_new_with_label(NULL, _("fruux"));
	google = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fruux), _("google"));
	others = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fruux), _("others"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(others), TRUE);

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
	g_object_set_data(G_OBJECT(box),"google",google);
	g_object_set_data(G_OBJECT(box),"fruux",fruux);
	gtk_box_pack_start (GTK_BOX (box), fruux, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (box), google, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (box), others, TRUE, TRUE, 2);

	gtk_widget_show_all(box);

	gtk_assistant_append_page (GTK_ASSISTANT(widget), box);
	gtk_assistant_set_page_type(GTK_ASSISTANT(widget), box, GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_title(GTK_ASSISTANT(widget), box, _("Select Server"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(widget), box, TRUE);
}

/**
 * newDialog - dialog to add a new server
 */
static void newDialog(GtkWidget *do_widget, gpointer trans){
	printfunc(__func__);

	GtkWidget 			*assistant = NULL;

	assistant = gtk_assistant_new ();
	gtk_window_set_destroy_with_parent(GTK_WINDOW(assistant), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (assistant), -1, 300);

	newDialogSelectType(assistant, trans);
	newDialogServerSettings(assistant, trans);
	newDialogUserCredentials(assistant, trans);
	newDialogOAuthCredentials(assistant, trans);
	newDialogConfirm(assistant, trans);

	gtk_widget_show_all(assistant);
	gtk_assistant_set_forward_page_func(GTK_ASSISTANT(assistant), newDialogSelectPage, assistant, NULL);

	/*		Connect Signales		*/
	g_signal_connect(G_OBJECT(assistant), "close", G_CALLBACK(newDialogClose), NULL);
	g_signal_connect(G_OBJECT(assistant), "cancel", G_CALLBACK(newDialogClose), NULL);
	g_signal_connect(G_OBJECT(assistant), "apply", G_CALLBACK(newDialogApply), trans);
}

/**
 * guiInit - build the basic GUI
 */
void guiInit(sqlite3 *ptr){
	printfunc(__func__);

	GtkWidget			*mainVBox, *mainToolbar, *mainStatusbar, *mainContent;
	GtkWidget			*addressbookWindow;
	GtkWidget			*contactBox, *contactWindow, *scroll;
	GtkWidget			*serverCombo;
	GtkWidget			*addContact, *delContact, *contactButtons, *contactsEdit, *editContact;
	GtkWidget			*ascContact, *descContact, *searchbar;
	GtkWidget			*emptyCard, *noContact;
	GtkToolItem			*comboItem, *prefItem, *aboutItem, *sep, *newServer, *syncItem, *exportItem;
	GtkTreeSelection	*bookSel, *contactSel;
	GSList 				*cleanUpList = g_slist_alloc();
	GtkEntryCompletion	*completion;
	ContactCards_trans_t		*transBook = NULL;
	ContactCards_trans_t		*transContact = NULL;
	ContactCards_trans_t		*transPref = NULL;
	ContactCards_trans_t		*transNew = NULL;
	ContactCards_trans_t		*transSync = NULL;
	ContactCards_trans_t		*transDelContact = NULL;
	ContactCards_trans_t		*transAddContact = NULL;
	ContactCards_trans_t		*transCompletion = NULL;
	ContactCards_trans_t		*transEditContact = NULL;

	gtk_init(NULL, NULL);

	mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(mainWindow), "ContactCards");
	gtk_window_set_default_size(GTK_WINDOW(mainWindow), 760,496);

	mainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	/*		Toolbar					*/
	mainToolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(mainToolbar), GTK_TOOLBAR_ICONS);

	newServer = gtk_tool_button_new(NULL, _("New"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(newServer), _("New"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (newServer), "document-new");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), newServer, -1);

	prefItem = gtk_tool_button_new(NULL, _("Preferences"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(prefItem), _("Preferences"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (prefItem), "preferences-system");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), prefItem, -1);

	exportItem = gtk_tool_button_new(NULL, _("Export"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(exportItem), _("Export"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (exportItem), "document-save");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), exportItem, -1);

	syncItem = gtk_tool_button_new(NULL, _("Refresh"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(syncItem), _("Refresh"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (syncItem), "view-refresh");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), syncItem, -1);

	serverCombo = comboInit(ptr);
	comboItem = gtk_tool_item_new();
	gtk_container_add(GTK_CONTAINER(comboItem), serverCombo);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), comboItem, -1);

	sep = gtk_separator_tool_item_new();
	gtk_tool_item_set_expand(sep, TRUE);
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(sep), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), sep, -1);
	aboutItem = gtk_tool_button_new(NULL, _("About"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(aboutItem), _("About"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (aboutItem), "help-about");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), aboutItem, -1);

	/*		Statusbar				*/
	mainStatusbar = gtk_statusbar_new();

	/*		mainContent				*/
	mainContent = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	/*		Addressbookstuff		*/
	addressbookWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(addressbookWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(addressbookWindow, 160, -1);
	addressbookList = gtk_tree_view_new();
	listInit(addressbookList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(addressbookList), FALSE);

	/*		Contactstuff			*/
	contactBox = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	contactWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(contactWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(contactWindow, 160, -1);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	emptyCard = gtk_grid_new();
	gtk_widget_set_halign(GTK_WIDGET(emptyCard), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(emptyCard), GTK_ALIGN_CENTER);
	noContact = gtk_image_new_from_icon_name("avatar-default-symbolic", GTK_ICON_SIZE_DIALOG);
	gtk_container_add(GTK_CONTAINER(emptyCard), noContact);
	gtk_container_add(GTK_CONTAINER(scroll), emptyCard);
	contactList = gtk_tree_view_new();
	listInit(contactList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(contactList), FALSE);
	contactsEdit =gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	contactButtons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	
	ascContact = gtk_button_new_from_icon_name("view-sort-ascending", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(ascContact), _("Sort contacts in ascending order"));
	gtk_container_add(GTK_CONTAINER(contactButtons), ascContact);
	descContact = gtk_button_new_from_icon_name("view-sort-descending", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(descContact), _("Sort contacts in descending order"));
	gtk_container_add(GTK_CONTAINER(contactButtons), descContact);
	delContact = gtk_button_new_from_icon_name("list-remove", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(delContact), _("Delete selected contact"));
	gtk_container_add(GTK_CONTAINER(contactButtons), delContact);
	addContact = gtk_button_new_from_icon_name("list-add", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(addContact), _("Add new contact"));
	gtk_container_add(GTK_CONTAINER(contactButtons), addContact);
	editContact = gtk_button_new_from_icon_name("gtk-edit", 1);
	gtk_widget_set_tooltip_text(GTK_WIDGET(editContact), _("Edit contact"));
	gtk_container_add(GTK_CONTAINER(contactButtons), editContact);
	completion = gtk_entry_completion_new ();
	gtk_entry_completion_set_popup_set_width(GTK_ENTRY_COMPLETION(completion), TRUE);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(contactList))));
	gtk_entry_completion_set_text_column(completion, 0);
	searchbar = gtk_entry_new();
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(searchbar), GTK_ENTRY_ICON_SECONDARY, "stock_search");
	gtk_entry_set_completion(GTK_ENTRY(searchbar), GTK_ENTRY_COMPLETION(completion));
	gtk_widget_set_vexpand(GTK_WIDGET(contactList), TRUE);
	gtk_container_add(GTK_CONTAINER(contactsEdit), contactWindow);
	gtk_container_add(GTK_CONTAINER(contactsEdit), searchbar);
	gtk_container_add(GTK_CONTAINER(contactsEdit), contactButtons);

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
	transContact->element = scroll;
	cleanUpList = g_slist_append(cleanUpList, transContact);
	contactSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(contactList));
	gtk_tree_selection_set_mode (contactSel, GTK_SELECTION_SINGLE);
	g_signal_connect(contactSel, "changed", G_CALLBACK(selContact), transContact);

	g_signal_connect(G_OBJECT(ascContact), "clicked", G_CALLBACK(listSortorderAsc), NULL);
	g_signal_connect(G_OBJECT(descContact), "clicked", G_CALLBACK(listSortorderDesc), NULL);

	transCompletion = g_new(ContactCards_trans_t, 1);
	transCompletion->db = ptr;
	transCompletion->element = scroll;
	cleanUpList = g_slist_append(cleanUpList, transCompletion);
	g_signal_connect(G_OBJECT(completion), "match-selected", G_CALLBACK(completionContact), transCompletion);

	transDelContact = g_new(ContactCards_trans_t, 1);
	cleanUpList = g_slist_append(cleanUpList, transDelContact);
	transDelContact->db = ptr;
	transDelContact->element = gtk_tree_view_get_selection(GTK_TREE_VIEW(contactList));
	g_signal_connect(G_OBJECT(delContact), "clicked", G_CALLBACK(contactDel), transDelContact);

	transAddContact = g_new(ContactCards_trans_t, 1);
	cleanUpList = g_slist_append(cleanUpList, transAddContact);
	transAddContact->db = ptr;
	transAddContact->element = gtk_tree_view_get_selection(GTK_TREE_VIEW(addressbookList));
	transAddContact->element2 = scroll;
	g_signal_connect(G_OBJECT(addContact), "clicked", G_CALLBACK(contactNew), transAddContact);

	transEditContact = g_new(ContactCards_trans_t, 1);
	cleanUpList = g_slist_append(cleanUpList, transEditContact);
	transEditContact->db = ptr;
	transEditContact->element = gtk_tree_view_get_selection(GTK_TREE_VIEW(contactList));
	transEditContact->element2 = scroll;
	g_signal_connect(G_OBJECT(editContact), "clicked", G_CALLBACK(contactEdit), transEditContact);

	g_signal_connect(G_OBJECT(mainWindow), "key_press_event", G_CALLBACK(guiKeyHandler), cleanUpList);
	g_signal_connect(G_OBJECT(mainWindow), "destroy", G_CALLBACK(guiExit), cleanUpList);
	g_signal_connect(G_OBJECT(prefItem), "clicked", G_CALLBACK(prefWindow), transPref);
	g_signal_connect(G_OBJECT(aboutItem), "clicked", G_CALLBACK(dialogAbout), NULL);
	g_signal_connect(G_OBJECT(newServer), "clicked", G_CALLBACK(newDialog), transNew);
	g_signal_connect(G_OBJECT(syncItem), "clicked", G_CALLBACK(syncServer), transSync);
	g_signal_connect(G_OBJECT(exportItem), "clicked", G_CALLBACK(dialogExportContacts), transPref);

	/*		Put it all together		*/
	gtk_box_pack_start(GTK_BOX(mainVBox), mainToolbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(addressbookWindow), addressbookList);
	gtk_container_add(GTK_CONTAINER(mainContent), addressbookWindow);
	gtk_container_add(GTK_CONTAINER(contactWindow), contactList);
	gtk_container_add(GTK_CONTAINER(contactBox), contactsEdit);
	gtk_container_add(GTK_CONTAINER(contactBox), scroll);
	gtk_container_add(GTK_CONTAINER(mainContent), contactBox);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainContent, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainStatusbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(mainWindow), mainVBox);
	gtk_widget_show_all(mainWindow);
}
