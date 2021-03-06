/* Copyright (c) 2013-2016 Florian L. <dev@der-flo.net>
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

#include <gtk/gtk.h>

#include "contactcards.h"

/**
 * guiRun - run the basic GUI
 */
void guiRun(sqlite3 *ptr){
	__PRINTFUNC__;

	addressbookTreeUpdate();
	contactsTreeUpdate(0,0);
	gtk_main();
}

/**
 * exitOnSignal - simple try, to exit properly on a signal
 */
gboolean exitOnSignal(gpointer data){
	__PRINTFUNC__;

	gtk_main_quit();
	g_slist_free_full(data, g_free);

	return TRUE;
}

/**
 * guiExit - exit the basic GUI and clean up
 */
void guiExit(GtkWidget *widget, gpointer data){
	__PRINTFUNC__;

	gtk_main_quit();
	g_slist_free_full(data, g_free);
}

/**
 * guiKeyHandler - control some kind of the basic GUI by keyboard
 */
void guiKeyHandler(GtkWidget *gui, GdkEventKey *event, gpointer data){
	__PRINTFUNC__;

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		guiExit(gui, data);
	}
}
/**
 * dialogKeyHandler - control some kind of a dialog by keyboard
 */
void dialogKeyHandler(GtkDialog *widget, GdkEventKey *event, gpointer data){
	__PRINTFUNC__;

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		gtk_dialog_response(widget, GTK_RESPONSE_DELETE_EVENT);
	}
}

/**
 * dialogAbout - display some basic stuff about this software
 */
static void dialogAbout(void){
	__PRINTFUNC__;

	GError			*error = NULL;
	GdkPixbuf		*pixbuf;

	pixbuf = gdk_pixbuf_new_from_file("artwork/icon_128.png", &error);
	if(error){
		verboseCC("[%s] something has gone wrong\n", __func__);
		verboseCC("%s\n", error->message);
	}

	gtk_show_about_dialog(NULL,
		"title", _("About ContactCards"),
		"program-name", "ContactCards",
		"comments", _("Address book written in C using DAV"),
		"website", "https://www.der-flo.net/contactcards/",
		"license", "GNU General Public License, version 2\nhttp://www.gnu.org/licenses/old-licenses/gpl-2.0.html",
		"version", VERSION,
		"logo", pixbuf,
		NULL);

	g_object_unref(pixbuf);
}

/**
 * selBook - select a single address book
 */
static void selBook(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GtkTreeModel			*model;
	GtkTreeIter				iter;
	int						selID = -1;
	int						selTyp = -1;

	if(g_mutex_trylock(&aBookTreeMutex) != TRUE){return;}
    verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.addressbookList)), &model, &iter)){
		gtk_tree_model_get (model, &iter, ID_COL, &selID, TYP_COL, &selTyp, -1);
		verboseCC("[%s] Typ: %d\tID:%d\n", __func__, selTyp, selID);
		g_mutex_unlock(&aBookTreeMutex);
        verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
		switch(selTyp){
			case 0:		/* Whole Server selected	*/
				if(selID == 0){
					contactsTreeUpdate(0, 0);
					calendarUpdate(0,0);
				} else {
					verboseCC("[%s] whole server\n", __func__);
					contactsTreeUpdate(0, selID);
					calendarUpdate(0, selID);
				}
				break;
			case 1:		/* Just one address book selected	*/
				contactsTreeUpdate(1, selID);
				calendarUpdate(1, selID);
				break;
			case 2:		/*	Favorites are selected			*/
				contactsTreeUpdate(2, 0);
				calendarUpdate(2, 0);
				break;
			case 3:		/*	Locales are selected			*/
				contactsTreeUpdate(3, 0);
				calendarUpdate(3, 0);
				break;
			default:
				verboseCC("[%s] something is strange around here\n", __func__);
				break;
		}
	}

}

/**
 * feedbackDialog
 *	@type			type of the dialog
 *	@msg			message which will be shown
 *
 *	This function gives the user a feedback via a dialog
 */
void feedbackDialog(int type, char *msg){
	__PRINTFUNC__;

	GtkWidget		*infoDia;

	infoDia = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT & GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, "%s", msg);
	gtk_dialog_run(GTK_DIALOG(infoDia));
	gtk_widget_destroy(infoDia);
}

/**
 * contactDel - delete a selected vCard
 */
static void contactDel(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkTreeStore		*store;
	GtkWidget			*dialog;
	int					selID, addrID, srvID;
	int					flag = 0;
	ne_session 			*sess = NULL;
	gint 				resp;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		int				isOAuth = 0;
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);

		addrID = getSingleInt(appBase.db, "contacts", "addressbookID", 1, "contactID", selID, "", "", "", "");
		srvID = getSingleInt(appBase.db, "addressbooks", "cardServer", 1, "addressbookID", addrID, "", "", "", "");

		flag = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", srvID, "", "", "", "");
		if((flag & CONTACTCARDS_ONE_WAY_SYNC) == CONTACTCARDS_ONE_WAY_SYNC){
			feedbackDialog(GTK_MESSAGE_WARNING, _("With One-Way-Sync enabled you are not allowed to do this!"));
			return;
		}

		verboseCC("[%s] %d\n",__func__, selID);

		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT & GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Do you really want to delete this contact?"));

		resp = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if (resp != GTK_RESPONSE_YES) return;

		while(g_mutex_trylock(&mutex) != TRUE){}
        verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);
		isOAuth = getSingleInt(appBase.db, "cardServer", "isOAuth", 1, "serverID", srvID, "", "", "", "");

		if(isOAuth){
			int 		ret = 0;
			ret = oAuthUpdate(appBase.db, srvID);
			if(ret != OAUTH_UP2DATE)
				goto failure;
		}

		sess = serverConnect(srvID);
		serverDelContact(appBase.db, sess, srvID, selID);
		serverDisconnect(sess, appBase.db, srvID);
		store = GTK_TREE_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(appBase.contactList)));
		gtk_tree_store_remove(store, &iter);

failure:
		g_mutex_unlock(&mutex);
        verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no contact selected."));
	}
}

/**
 * contactDelcb - Callback for contactDel()
 */
static void contactDelcb(GtkMenuItem *menuitem, gpointer     user_data){
	__PRINTFUNC__;

	contactDel(NULL, NULL);
}

/**
 * contactEditPostalItem - Add a postal address to edit a vCard
 */
static int contactEditPostalItem(GtkWidget *grid, GSList *list, int line, char *value){
	__PRINTFUNC__;

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
	g_strfreev(postalPtr);

	label = gtk_label_new(_("post office box"));
	input = gtk_entry_new_with_buffer(boxBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(grid), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 2, line++, 5, 1);
	boxItem->itemID = CARDTYPE_ADR_OFFICE_BOX;
	boxItem->element = boxBuf;
	elements = g_slist_append(elements, boxItem);

	label = gtk_label_new(_("extended address"));
	input = gtk_entry_new_with_buffer(extBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(grid), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 2, line++, 5, 1);
	extItem->itemID = CARDTYPE_ADR_EXT_ADDR;
	extItem->element = extBuf;
	elements = g_slist_append(elements, extItem);

	label = gtk_label_new(_("street"));
	input = gtk_entry_new_with_buffer(streetBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(grid), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 2, line++, 5, 1);
	streetItem->itemID = CARDTYPE_ADR_STREET;
	streetItem->element = streetBuf;
	elements = g_slist_append(elements, streetItem);

	label = gtk_label_new(_("city"));
	input = gtk_entry_new_with_buffer(cityBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(grid), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 2, line++, 5, 1);
	cityItem->itemID = CARDTYPE_ADR_CITY;
	cityItem->element = cityBuf;
	elements = g_slist_append(elements, cityItem);

	label = gtk_label_new(_("region"));
	input = gtk_entry_new_with_buffer(regBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(grid), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 2, line++, 5, 1);
	regItem->itemID = CARDTYPE_ADR_REGION;
	regItem->element = regBuf;
	elements = g_slist_append(elements, regItem);

	label = gtk_label_new(_("zip"));
	input = gtk_entry_new_with_buffer(zipBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(grid), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 2, line++, 5, 1);
	zipItem->itemID = CARDTYPE_ADR_ZIP;
	zipItem->element = zipBuf;
	elements = g_slist_append(elements, zipItem);

	label = gtk_label_new(_("country"));
	input = gtk_entry_new_with_buffer(countryBuf);
	gtk_widget_set_margin_bottom(input, 12);
	gtk_grid_attach(GTK_GRID(grid), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), input, 2, line++, 5, 1);
	countryItem->itemID = CARDTYPE_ADR_COUNTRY;
	countryItem->element = countryBuf;
	elements = g_slist_append(elements, countryItem);

	eleList->itemID = CARDTYPE_ADR;
	eleList->element = elements;
	list = g_slist_append(list, eleList);

	return line;
}

/**
 * contactEditSingleItem - Add a single line value to edit a vCard
 * Here a single linked list is added to a single linked list, to
 * implement TYP-stuff in the future
 */
static int contactEditSingleItem(GtkWidget *grid, GSList *list, int type, int line, char *value){
	__PRINTFUNC__;

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
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(grid), input, 2, line++, 5, 1);
    switch(type)
    {
        case CARDTYPE_TEL:
            gtk_entry_set_input_purpose(GTK_ENTRY(input), GTK_INPUT_PURPOSE_PHONE);
            break;
        case CARDTYPE_EMAIL:
            gtk_entry_set_input_purpose(GTK_ENTRY(input), GTK_INPUT_PURPOSE_EMAIL);
            break;
        default:
            break;
    }
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
	__PRINTFUNC__;

	GtkWidget				*input;
	GtkTextBuffer			*buf;
	GSList					*elements;
	ContactCards_item_t		*item, *eleList;

	item = g_new(ContactCards_item_t, 1);
	eleList = g_new(ContactCards_item_t, 1);
	buf = gtk_text_buffer_new(NULL);

	elements = g_slist_alloc();

	gtk_text_buffer_set_text(buf, g_strcompress(g_strstrip(value)), -1);

	input = gtk_text_view_new_with_buffer(buf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_widget_set_hexpand(GTK_WIDGET(input), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(input), TRUE);
	gtk_grid_attach(GTK_GRID(grid), input, 2, line++, 5, 5);
	item->itemID = type;
	item->element = buf;
	elements = g_slist_append(elements, item);

	eleList->itemID = type;
	eleList->element = elements;
	list = g_slist_append(list, eleList);

	return line;
}

/**
 * viewCleaner - clean up the area to display a new content
 */
void viewCleaner(GtkWidget *widget){
	__PRINTFUNC__;

	GList				*children, *child;

	children = gtk_container_get_children(GTK_CONTAINER(widget));
	for(child = children; child != NULL; child = g_list_next(child))
		gtk_widget_destroy(GTK_WIDGET(child->data));
	g_list_free(children);
}

/**
 * collectionCreate - Signalcallback to create a new collection
 */
static void collectionCreate(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	ContactCards_item_t		*data = trans;
	char					*colName = NULL;
	int						isOAuth = 0;
	ne_session 				*sess = NULL;

	if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(data->element)) == 0){
		viewCleaner(appBase.contactView);
		g_free(data);
		feedbackDialog(GTK_MESSAGE_ERROR, _("Unable to create an address book without a name"));
		return;
	}
	colName = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(data->element)));

	viewCleaner(appBase.contactView);

	while(g_mutex_trylock(&mutex) != TRUE){}
    verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);
	isOAuth = getSingleInt(appBase.db, "cardServer", "isOAuth", 1, "serverID", data->itemID, "", "", "", "");
	if(isOAuth){
		int 		ret = 0;
		ret = oAuthUpdate(appBase.db,  data->itemID);
		if(ret != OAUTH_UP2DATE){
			g_mutex_unlock(&mutex);
            verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
			g_free(data);
			g_free(colName);
			return;
		}
	}

	sess = serverConnect(data->itemID);
	serverCreateCollection(sess, data->itemID, colName);
	serverDisconnect(sess, appBase.db, data->itemID);
	addressbookTreeUpdate();

	g_mutex_unlock(&mutex);
    verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
	g_free(data);
	g_free(colName);
}

/**
 * collectionDiscard - Signalcallback to delete obsolete stuff
 */
static void collectionDiscard(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	ContactCards_item_t		*data = trans;

	viewCleaner(appBase.contactView);

	g_free(data);

}

/**
 * createNewCollectionCard - Frontend for the user to create a new collection
 */
static GtkWidget *createNewCollectionCard(int srvID){
	__PRINTFUNC__;

	GtkWidget				*collection;
	GtkWidget				*discardBtn, *saveBtn, *row;
	GtkWidget				*label, *input;
	GtkEntryBuffer			*colNameBuf;
	int						line = 1;
	char					*markup;
	ContactCards_item_t		*transCol = NULL;

	transCol = g_new(ContactCards_item_t, 1);

	collection = gtk_grid_new();

	gtk_widget_set_hexpand(GTK_WIDGET(collection), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(collection), TRUE);
	gtk_widget_set_halign(GTK_WIDGET(collection), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(collection), GTK_ALIGN_START);

	row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

	discardBtn = gtk_button_new_with_label(_("Discard"));
	saveBtn = gtk_button_new_with_label(_("Save"));
	gtk_box_pack_end(GTK_BOX(row), discardBtn, FALSE, FALSE, 1);
	gtk_box_pack_end(GTK_BOX(row), saveBtn, FALSE, FALSE, 1);
	gtk_grid_attach(GTK_GRID(collection), row, 0, line++, 3, 1);

	colNameBuf = gtk_entry_buffer_new(NULL, -1);

	line++;
	label = gtk_label_new(NULL);
	markup =  g_markup_printf_escaped ("<span size=\"18000\"><b>Create new address book</b></span>");
	gtk_label_set_markup (GTK_LABEL(label), markup);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_grid_attach(GTK_GRID(collection), label, 0, line++, 2, 1);
	line++;

	label = gtk_label_new(_("Address book name"));
	input = gtk_entry_new_with_buffer(colNameBuf);
	gtk_grid_attach(GTK_GRID(collection), label, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(collection), input, 1, line++, 2, 1);

	transCol->itemID = srvID;
	transCol->element = colNameBuf;

	/*		Connect Signales		*/
	g_signal_connect(G_OBJECT(saveBtn), "clicked", G_CALLBACK(collectionCreate), transCol);
	g_signal_connect(G_OBJECT(discardBtn), "clicked", G_CALLBACK(collectionDiscard), transCol);

	return collection;
}

/**
 * cbExportContacts - Callback to export contacts
 */
static void cbExportContacts(int type, int sel){
	__PRINTFUNC__;

	GtkWidget					*dirChooser;
	int							result;
	char						*path = NULL;

	dirChooser = gtk_file_chooser_dialog_new(_("Export Contacts"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);

	g_signal_connect(G_OBJECT(dirChooser), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	result = gtk_dialog_run(GTK_DIALOG(dirChooser));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dirChooser));
			exportContactsCB(appBase.db, path, type, sel);
			g_free(path);
			break;
		default:
			break;
	}
	gtk_widget_destroy(dirChooser);
}

/**
 * dialogExportBirthdays - dialog to export vCards
 */
static void dialogExportBirthdays(int type, int id){
	__PRINTFUNC__;

	GtkWidget					*dirChooser;
	int							result;
	char						*path = NULL;

	dirChooser = gtk_file_chooser_dialog_new(_("Export Contacts"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);

	g_signal_connect(G_OBJECT(dirChooser), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	result = gtk_dialog_run(GTK_DIALOG(dirChooser));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dirChooser));
			exportBirthdays(type, id, path);
			g_free(path);
			break;
		default:
			break;
	}
	gtk_widget_destroy(dirChooser);
}

/**
 * cbAddrBookExportBirthdays - Callback from popup-menu to export birthdays
 */
void cbAddrBookExportBirthdays(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	int				abID = 0;

	abID = GPOINTER_TO_INT(data);
	dialogExportBirthdays(1, abID);
}

/**
 * cbSrvExportBirthdays - Callback from popup-menu to export birthdays
 */
void cbSrvExportBirthdays(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	int				srvID = 0;

	srvID = GPOINTER_TO_INT(data);
	dialogExportBirthdays(0, srvID);
}

/**
 * cbExportContactAll - Callback to export contacts all contacts
 */
void cbExportContactAll(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	cbExportContacts(0, 0);
}

/**
 * cbExportContactSrv - Callback to export contacts from server
 */
void cbExportContactSrv(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	int				srvID = 0;

	srvID = GPOINTER_TO_INT(data);
	cbExportContacts(1, srvID);
}

/**
 * cbExportContactFav - Callback to export contacts marked as favorites
 */
void cbExportContactFav(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	cbExportContacts(4, 0);
}

/**
 * cbExportContactLoc - Callback to export local contacts
 */
void cbExportContactLoc(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	cbExportContacts(5, 0);
}

/**
 * cbExportContactABook - Callback to export contacts from address book
 */
void cbExportContactABook(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	int				aID = 0;

	aID = GPOINTER_TO_INT(data);
	cbExportContacts(2, aID);
}

/**
 * cbFavoritesExportBirthdays - Callback from popup-menu to export birthdays
 */
void cbFavoritesExportBirthdays(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	dialogExportBirthdays(2, 0);
}

/**
 * cbLocalesExportBirthdays - Callback from popup-menu to export birthdays
 */
void cbLocalesExportBirthdays(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	dialogExportBirthdays(3, 0);
}

/**
 * createNewCollection - Callback from popup-menu to create a new collection
 */
void createNewCollection(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	int				srvID = 0;
	GtkWidget		*collection;
	int				flag = 0;

	srvID = GPOINTER_TO_INT(data);
	verboseCC("[%s] new collection on %d\n", __func__, srvID);

	flag = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", srvID, "", "", "", "");
	if((flag & CONTACTCARDS_ONE_WAY_SYNC) == CONTACTCARDS_ONE_WAY_SYNC){
		feedbackDialog(GTK_MESSAGE_WARNING, _("With One-Way-Sync enabled you are not allowed to do this!"));
		return;
	}

	collection = createNewCollectionCard(srvID);
	gtk_widget_show_all(collection);
	viewCleaner(appBase.contactView);
	gtk_container_add(GTK_CONTAINER(appBase.contactView), collection);

}

/**
 * getWidgetFromID - returns the type parameter
 */
static GtkWidget *getWidgetFromID(int id){
	__PRINTFUNC__;

	GtkWidget		*new;
	GString			*value;

	value = g_string_new(NULL);

	if(id & TYPE_HOME){
		g_string_append(value, _("Home "));
	}
	if(id & TYPE_WORK){
		g_string_append(value, _("Work "));
	}
	new = gtk_label_new(value->str);
	g_string_free(value, TRUE);

	return new;
}

/**
 * edgy2roundCB - Callback to round off the corners
 */
static gboolean edgy2roundCB(GtkWidget *widget, GdkEventConfigure *event, gpointer *data){
	__PRINTFUNC__;

	cairo_t			*cr;
	GError			*error = NULL;
	GdkPixbuf		*pixbuf = NULL;
	int				w = 0,
					h = 0,
					f = 0;

	cr = gdk_cairo_create (gtk_widget_get_window (widget));
	pixbuf = gdk_pixbuf_new_from_stream((GInputStream *)data, NULL, &error);
	if(error){
		verboseCC("[%s] %s\n", __func__, error->message);
		pixbuf = gdk_pixbuf_new_from_data((guchar *) data, GDK_COLORSPACE_RGB, TRUE, 8, 96,128, 3*96, NULL, NULL);
	}
	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);

	if(w > 100){
		GdkPixbuf		*scaled = NULL;
		f = w/100;
		scaled = gdk_pixbuf_scale_simple(pixbuf, w/f, h/f, GDK_INTERP_TILES);
		w = 100;
		gdk_cairo_set_source_pixbuf (cr, scaled, 2.5, 2.5);
		g_object_unref(scaled);
	} else if (h > 100){
		GdkPixbuf		*scaled = NULL;
		f = h/100;
		scaled = gdk_pixbuf_scale_simple(pixbuf, w/f, h/f, GDK_INTERP_TILES);
		h = 100;
		gdk_cairo_set_source_pixbuf (cr, scaled, 2.5, 2.5);
		g_object_unref(scaled);
	} else {
		gdk_cairo_set_source_pixbuf (cr, pixbuf, 2.5, 2.5);
	}

	cairo_new_sub_path(cr);
	cairo_arc(cr, 90.0, 10.0, 5.0, -3.14/2, 0);			// upper right
	cairo_arc(cr,90.0,90.0, 5.0, 0, 3.14/2);			// lower right
	cairo_arc(cr, 10.0,90.0, 5.0, 3.14/2, 3.14);		// lower left
	cairo_arc(cr, 10.0, 10.0, 5.0, 3.14, 3*3.14/2);		// upper left
	cairo_close_path(cr);

	cairo_clip(cr);
	cairo_paint (cr);
	cairo_destroy (cr);

	return TRUE;
}

/**
 * buildNewCard - display the data of a selected vCard
 */
static GtkWidget *buildNewCard(sqlite3 *ptr, int selID){
	__PRINTFUNC__;

	GtkWidget			*card;
	GtkWidget			*frame;
	GtkWidget			*photo, *fn, *bday;
	GtkWidget			*typ, *content;
	GSList				*list;
	char				*vData = NULL;
	int					line = 0;
	char				*markup;
	ContactCards_pix_t	*tmp = NULL;

	frame = gtk_frame_new(NULL);

	gtk_widget_set_hexpand(GTK_WIDGET(frame), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(frame), TRUE);

	card = gtk_grid_new();
	vData = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", selID, "", "", "", "", "", 0);
	if(vData == NULL)
		return card;

	gtk_widget_set_hexpand(GTK_WIDGET(card), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(card), TRUE);
	gtk_widget_set_halign(GTK_WIDGET(card), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(card), GTK_ALIGN_START);

	/*		PHOTO		*/
	tmp = getCardPhoto(vData);
	if(tmp->size == 0){
		char			*cmp = NULL;
		cmp = getSingleCardAttribut(CARDTYPE_SHOWAS, vData);
		if(cmp){
			char		*down = g_utf8_strdown(cmp, strlen(cmp));
			if(g_regex_match_simple ("company$", down, 0,0) == TRUE){
				photo = gtk_image_new_from_icon_name("stock_home", GTK_ICON_SIZE_DIALOG);
			}else{
				photo = gtk_image_new_from_icon_name("avatar-default-symbolic", GTK_ICON_SIZE_DIALOG);
			}
			g_free(cmp);
			g_free(down);
		} else {
			photo = gtk_image_new_from_icon_name("avatar-default-symbolic", GTK_ICON_SIZE_DIALOG);
		}
	} else {
		GInputStream	*ginput = g_memory_input_stream_new_from_data(tmp->pixel, tmp->size, NULL);
		photo = gtk_drawing_area_new();
		g_signal_connect (photo, "draw", G_CALLBACK(edgy2roundCB), (gpointer *)ginput);
	}
	gtk_widget_set_size_request(GTK_WIDGET(photo), 102, 102);
	gtk_widget_set_margin_start(photo, 18);
	gtk_widget_set_margin_top(photo, 6);
	gtk_widget_set_halign(photo, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(card), photo, 0, line, 1, 3);
	g_free(tmp);

	/*		FN		*/
	fn = gtk_label_new(NULL);
	markup = g_markup_printf_escaped ("<span size=\"18000\"><b>%s</b></span>", getSingleCardAttribut(CARDTYPE_FN, vData));
	gtk_label_set_markup (GTK_LABEL(fn), markup);
	gtk_label_set_line_wrap(GTK_LABEL(fn), TRUE);
	gtk_widget_set_margin_start(fn, 12);
	gtk_widget_set_margin_end(fn, 12);
	gtk_widget_set_margin_top(fn, 6);
	gtk_widget_set_halign(fn, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(card), fn, 1, line++, 1, 2);
	line++;
	/*		BDAY		*/
	bday = gtk_label_new(getSingleCardAttribut(CARDTYPE_BDAY, vData));
	gtk_widget_set_margin_start(bday, 12);
	gtk_widget_set_margin_end(bday, 12);
	gtk_widget_set_margin_top(bday, 2);
	gtk_widget_set_halign(bday, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(card), bday, 1, line++, 1, 1);

	/*		Anniversary		*/
	list = getMultipleCardAttribut(CARDTYPE_ANNIVERSARY, vData, FALSE);
	if (g_slist_length(list) > 1){
		typ = gtk_label_new(_("Anniversary"));
		gtk_widget_set_margin_start(typ, 12);
		gtk_widget_set_margin_end(typ, 12);
		gtk_widget_set_margin_top(typ, 18);
		gtk_widget_set_halign(typ, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
		while(list){
			GSList					*next = list->next;
			char					*value = (char *) list->data;
			if(value != NULL){
				GtkEntryBuffer	*val = gtk_entry_buffer_new(NULL, -1);
				gtk_entry_buffer_set_text(val, g_strstrip(value), -1);
				content = gtk_entry_new_with_buffer(val);
				gtk_editable_set_editable(GTK_EDITABLE(content), FALSE);
				gtk_widget_set_margin_start(content, 12);
				gtk_widget_set_margin_end(content, 12);
				gtk_widget_set_margin_top(content, 6);
				gtk_widget_set_size_request(GTK_WIDGET(content), 322, -1);
				gtk_widget_set_hexpand(content, TRUE);
				gtk_widget_set_halign(GTK_WIDGET(content), GTK_ALIGN_START);
				gtk_grid_attach(GTK_GRID(card), content, 1, line++, 1, 1);
			}
			list = next;
		}
	}
	g_slist_free_full(list, g_free);

	/*		Phone		*/
	list = getMultipleCardAttribut(CARDTYPE_TEL, vData, TRUE);
	if (g_slist_length(list) > 1){
		typ = gtk_label_new(_("Phone"));
		gtk_widget_set_margin_start(typ, 12);
		gtk_widget_set_margin_end(typ, 12);
		gtk_widget_set_margin_top(typ, 18);
		gtk_widget_set_halign(typ, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
		while(list){
			GSList					*next = list->next;
			ContactCards_item_t 	*item;
			char					*value = NULL;
			if(list->data){
				item = (ContactCards_item_t *) list->data;
				value = (char *) item->element;
				if(value != NULL){
					GtkEntryBuffer	*val = gtk_entry_buffer_new(NULL, -1);
					GtkWidget		*attrType;
					attrType = getWidgetFromID(item->itemID);
					gtk_widget_set_margin_end(attrType, 6);
					gtk_widget_set_margin_top(attrType, 6);
					gtk_grid_attach(GTK_GRID(card), attrType, 0, line, 1, 1);
					gtk_widget_set_halign(GTK_WIDGET(attrType), GTK_ALIGN_END);
					gtk_entry_buffer_set_text(val, g_strstrip(value), -1);
					content = gtk_entry_new_with_buffer(val);
					gtk_editable_set_editable(GTK_EDITABLE(content), FALSE);
					gtk_widget_set_margin_start(content, 12);
					gtk_widget_set_margin_end(content, 12);
					gtk_widget_set_margin_top(content, 6);
					gtk_widget_set_size_request(GTK_WIDGET(content), 322, -1);
					gtk_widget_set_hexpand(content, TRUE);
					gtk_widget_set_halign(GTK_WIDGET(content), GTK_ALIGN_START);
					gtk_grid_attach(GTK_GRID(card), content, 1, line++, 1, 1);
				}
			}
			list = next;
		}
	}
	g_slist_free_full(list, g_free);

	/*		Address		*/
	list = getMultipleCardAttribut(CARDTYPE_ADR, vData, TRUE);
	if (g_slist_length(list) > 1){
		typ = gtk_label_new(_("Address"));
		gtk_widget_set_margin_start(typ, 12);
		gtk_widget_set_margin_end(typ, 12);
		gtk_widget_set_margin_top(typ, 18);
		gtk_widget_set_halign(typ, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
		while(list){
			GSList					*next = list->next;
			ContactCards_item_t 	*item;
			char					*value;
			if(list->data){
				item = (ContactCards_item_t *) list->data;
				value = (char *) item->element;
				if(value != NULL){
					GtkTextBuffer	*val = gtk_text_buffer_new(NULL);
					GtkWidget		*attrType;
					GtkWidget		*mapBtn;
					GtkWidget		*image;
					char			*mapUrl;
					char			*valCpy;
					valCpy = g_strndup(value, strlen(value));
					image = gtk_image_new_from_icon_name("go-jump", GTK_ICON_SIZE_SMALL_TOOLBAR);
					mapBtn = gtk_link_button_new("");
					if((appBase.flags & USE_GOOGLE) == USE_GOOGLE) {
					mapUrl = g_strconcat("https://www.google.com/maps?q=", g_uri_escape_string(g_strdelimit(valCpy, ";", ' '), NULL, TRUE), NULL);
					} else {
						mapUrl = g_strconcat("https://www.openstreetmap.org/search?query=", g_uri_escape_string(g_strdelimit(valCpy, ";", ' '), NULL, TRUE), "&polygon=1", NULL);
					}
					gtk_link_button_set_uri(GTK_LINK_BUTTON(mapBtn), mapUrl);
					gtk_button_set_image(GTK_BUTTON(mapBtn), image);
					attrType = getWidgetFromID(item->itemID);
					gtk_widget_set_margin_end(attrType, 6);
					gtk_widget_set_margin_top(attrType, 6);
					gtk_grid_attach(GTK_GRID(card), attrType, 0, line, 1, 1);
					gtk_grid_attach(GTK_GRID(card), mapBtn, 2, line, 1, 1);
					gtk_widget_set_halign(GTK_WIDGET(attrType), GTK_ALIGN_END);
					gtk_widget_set_valign(GTK_WIDGET(attrType), GTK_ALIGN_START);
					gtk_text_buffer_set_text(val, g_strstrip(g_strdelimit(value, ";", '\n')), -1);
					content = gtk_text_view_new_with_buffer(val);
					gtk_text_view_set_editable(GTK_TEXT_VIEW(content), FALSE);
					gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(content), GTK_WRAP_WORD);
					gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(content), GTK_TEXT_WINDOW_LEFT, 5);
					gtk_widget_set_margin_start(content, 12);
					gtk_widget_set_margin_end(content, 12);
					gtk_widget_set_margin_top(content, 6);
					gtk_widget_set_size_request(GTK_WIDGET(content), 322, -1);
					gtk_widget_set_hexpand(content, TRUE);
					gtk_widget_set_halign(GTK_WIDGET(content), GTK_ALIGN_START);
					gtk_grid_attach(GTK_GRID(card), content, 1, line++, 1, 1);
					g_free(valCpy);
					g_free(mapUrl);
				}
			}
			list = next;
		}
	}
	g_slist_free_full(list, g_free);

	/*		EMail		*/
	list = getMultipleCardAttribut(CARDTYPE_EMAIL, vData, FALSE);
	if (g_slist_length(list) > 1){
		typ = gtk_label_new(_("EMail"));
		gtk_widget_set_margin_start(typ, 12);
		gtk_widget_set_margin_end(typ, 12);
		gtk_widget_set_margin_top(typ, 18);
		gtk_widget_set_halign(typ, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
		while(list){
			GSList					*next = list->next;
			char					*value = (char *) list->data;
			if(value != NULL){
				GtkEntryBuffer	*val = gtk_entry_buffer_new(NULL, -1);
				gtk_entry_buffer_set_text(val, g_strstrip(value), -1);
				content = gtk_entry_new_with_buffer(val);
				gtk_editable_set_editable(GTK_EDITABLE(content), FALSE);
				gtk_widget_set_margin_start(content, 12);
				gtk_widget_set_margin_end(content, 12);
				gtk_widget_set_margin_top(content, 6);
				gtk_widget_set_size_request(GTK_WIDGET(content), 322, -1);
				gtk_widget_set_hexpand(content, TRUE);
				gtk_widget_set_halign(GTK_WIDGET(content), GTK_ALIGN_START);
				gtk_grid_attach(GTK_GRID(card), content, 1, line++, 1, 1);
			}
			list = next;
		}
	}
	g_slist_free_full(list, g_free);

	/*		Url		*/
	list = getMultipleCardAttribut(CARDTYPE_URL, vData, FALSE);
	if (g_slist_length(list) > 1){
		typ = gtk_label_new(_("URL"));
		gtk_widget_set_margin_start(typ, 12);
		gtk_widget_set_margin_end(typ, 12);
		gtk_widget_set_margin_top(typ, 18);
		gtk_widget_set_halign(typ, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
		while(list){
			GSList					*next = list->next;
			char					*value = (char *) list->data;
			if(value != NULL){
				GtkEntryBuffer	*val = gtk_entry_buffer_new(NULL, -1);
				gtk_entry_buffer_set_text(val, g_strstrip(value), -1);
				content = gtk_entry_new_with_buffer(val);
				gtk_editable_set_editable(GTK_EDITABLE(content), FALSE);
				gtk_widget_set_margin_start(content, 12);
				gtk_widget_set_margin_end(content, 12);
				gtk_widget_set_margin_top(content, 6);
				gtk_widget_set_size_request(GTK_WIDGET(content), 322, -1);
				gtk_widget_set_hexpand(content, TRUE);
				gtk_widget_set_halign(GTK_WIDGET(content), GTK_ALIGN_START);
				gtk_grid_attach(GTK_GRID(card), content, 1, line++, 1, 1);
			}
			list = next;
		}
	}
	g_slist_free_full(list, g_free);

	/*		Note		*/
	list = getMultipleCardAttribut(CARDTYPE_NOTE, vData, FALSE);
	if (g_slist_length(list) > 1){
		typ = gtk_label_new(_("Note"));
		gtk_widget_set_margin_start(typ, 12);
		gtk_widget_set_margin_end(typ, 12);
		gtk_widget_set_margin_top(typ, 18);
		gtk_widget_set_halign(typ, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
		while(list){
			GSList					*next = list->next;
			char					*value = (char *) list->data;
			if(value != NULL){
				GtkTextBuffer	*val = gtk_text_buffer_new(NULL);
				GtkWidget		*scroller = NULL;
                GtkRequisition  natSize;
				scroller = gtk_scrolled_window_new(NULL, NULL);
				gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
				gtk_text_buffer_set_text(val, g_strcompress(g_strstrip(value)), -1);
				content = gtk_text_view_new_with_buffer(val);
				gtk_text_view_set_editable(GTK_TEXT_VIEW(content), FALSE);
				gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(content), GTK_WRAP_WORD);
				gtk_widget_set_margin_start(scroller, 12);
				gtk_widget_set_margin_end(scroller, 12);
				gtk_widget_set_margin_top(scroller, 6);
				gtk_widget_get_preferred_size(GTK_WIDGET(content), NULL, &natSize);
				gtk_widget_set_size_request(GTK_WIDGET(scroller), 322, natSize.width + 35);
				gtk_widget_set_hexpand(scroller, TRUE);
				gtk_widget_set_halign(GTK_WIDGET(scroller), GTK_ALIGN_START);
				gtk_container_add(GTK_CONTAINER(scroller), content);
				gtk_grid_attach(GTK_GRID(card), scroller, 1, line++, 1, 1);
			}
			list = next;
		}
	}
	g_slist_free_full(list, g_free);

	/*		Key		*/
	list = getMultipleCardAttribut(CARDTYPE_KEY, vData, FALSE);
	if (g_slist_length(list) > 1){
		typ = gtk_label_new(_("Key"));
		gtk_widget_set_margin_start(typ, 12);
		gtk_widget_set_margin_end(typ, 12);
		gtk_widget_set_margin_top(typ, 18);
		gtk_widget_set_halign(typ, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
		while(list){
			GSList					*next = list->next;
			char					*value = (char *) list->data;
			if(value != NULL){
				GtkTextBuffer	*val = gtk_text_buffer_new(NULL);
				GtkWidget		*scroller = NULL;
                GtkRequisition  natSize;
				scroller = gtk_scrolled_window_new(NULL, NULL);
				gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
				gtk_text_buffer_set_text(val, g_strcompress(g_strstrip(value)), -1);
				content = gtk_text_view_new_with_buffer(val);
				gtk_text_view_set_editable(GTK_TEXT_VIEW(content), FALSE);
				gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(content), GTK_WRAP_WORD);
				gtk_widget_set_margin_start(scroller, 12);
				gtk_widget_set_margin_end(scroller, 12);
				gtk_widget_set_margin_top(scroller, 6);
                gtk_widget_get_preferred_size(GTK_WIDGET(content), NULL, &natSize);
				gtk_widget_set_size_request(GTK_WIDGET(scroller), 322, natSize.height +35);
				gtk_widget_set_hexpand(scroller, TRUE);
				gtk_widget_set_halign(GTK_WIDGET(scroller), GTK_ALIGN_START);
				gtk_container_add(GTK_CONTAINER(scroller), content);
				gtk_grid_attach(GTK_GRID(card), scroller, 1, line++, 1, 1);
			}
			list = next;
		}
	}
	g_slist_free_full(list, g_free);

	/*		Public Calendar URL		*/
	list = getMultipleCardAttribut(CARDTYPE_CALURI, vData, FALSE);
	if (g_slist_length(list) > 1){
		typ = gtk_label_new(_("Public Calendar URL"));
		gtk_widget_set_margin_start(typ, 12);
		gtk_widget_set_margin_end(typ, 12);
		gtk_widget_set_margin_top(typ, 18);
		gtk_widget_set_halign(typ, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
		while(list){
			GSList					*next = list->next;
			char					*value = (char *) list->data;
			if(value != NULL){
				GtkEntryBuffer	*val = gtk_entry_buffer_new(NULL, -1);
				gtk_entry_buffer_set_text(val, g_strstrip(value), -1);
				content = gtk_entry_new_with_buffer(val);
				gtk_editable_set_editable(GTK_EDITABLE(content), FALSE);
				gtk_widget_set_margin_start(content, 12);
				gtk_widget_set_margin_end(content, 12);
				gtk_widget_set_margin_top(content, 6);
				gtk_widget_set_size_request(GTK_WIDGET(content), 322, -1);
				gtk_widget_set_hexpand(content, TRUE);
				gtk_widget_set_halign(GTK_WIDGET(content), GTK_ALIGN_START);
				gtk_grid_attach(GTK_GRID(card), content, 1, line++, 1, 1);
			}
			list = next;
		}
	}
	g_slist_free_full(list, g_free);

	/*		Free/Busy Calendar URL		*/
	list = getMultipleCardAttribut(CARDTYPE_FBURL, vData, FALSE);
	if (g_slist_length(list) > 1){
		typ = gtk_label_new(_("Free/Busy Calendar URL"));
		gtk_widget_set_margin_start(typ, 12);
		gtk_widget_set_margin_end(typ, 12);
		gtk_widget_set_margin_top(typ, 18);
		gtk_widget_set_halign(typ, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
		while(list){
			GSList					*next = list->next;
			char					*value = (char *) list->data;
			if(value != NULL){
				GtkEntryBuffer	*val = gtk_entry_buffer_new(NULL, -1);
				gtk_entry_buffer_set_text(val, g_strstrip(value), -1);
				content = gtk_entry_new_with_buffer(val);
				gtk_editable_set_editable(GTK_EDITABLE(content), FALSE);
				gtk_widget_set_margin_start(content, 12);
				gtk_widget_set_margin_end(content, 12);
				gtk_widget_set_margin_top(content, 6);
				gtk_widget_set_size_request(GTK_WIDGET(content), 322, -1);
				gtk_widget_set_hexpand(content, TRUE);
				gtk_widget_set_halign(GTK_WIDGET(content), GTK_ALIGN_START);
				gtk_grid_attach(GTK_GRID(card), content, 1, line++, 1, 1);
			}
			list = next;
		}
	}
	g_slist_free_full(list, g_free);

	g_free(vData);

	content = gtk_label_new(NULL);
	gtk_grid_attach(GTK_GRID(card), content, 1, line++, 1, 1);

	gtk_container_add(GTK_CONTAINER(frame), card);

	return frame;
}

/**
 * contactEditDiscard - discard the changes on a vCard
 */
static void contactEditDiscard(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GtkWidget		*card;

	card = buildNewCard(appBase.db, ((ContactCards_add_t *)trans)->editID);
	gtk_widget_show_all(card);
	viewCleaner(appBase.contactView);
	gtk_container_add(GTK_CONTAINER(appBase.contactView), card);

	g_slist_free_full(((ContactCards_add_t *)trans)->list, g_free);
	g_free(trans);
}

/**
 * pushingCard - Threadfunction to push a vCard
 */
static void *pushingCard(void *trans){
	__PRINTFUNC__;

	ContactCards_add_t			*value = trans;
	char						*vCard = NULL;
	char						*dbCard = NULL;
	int							oldID = value->editID;
	int							addrID = value->aID;

	while(g_mutex_trylock(&mutex) != TRUE){}
    verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);
	if(oldID){
		dbCard = getSingleChar(appBase.db, "contacts", "vCard", 1, "contactID", oldID, "", "", "", "", "", 0);
		vCard = mergeCards(value->list, dbCard);
	} else {
		vCard = buildCard(value->list);
	}

	if(pushCard(appBase.db, vCard, addrID, 1, oldID) == 1){
		dbRemoveItem(appBase.db, "contacts", 2, "", "", "contactID", oldID);
	} else {
		feedbackDialog(GTK_MESSAGE_ERROR, _("Unable to save changes"));
	}

	g_slist_free_full(value->list, g_free);
	g_free(trans);
	g_free(vCard);
	g_free(dbCard);

	g_mutex_unlock(&mutex);
    verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
	return NULL;
}

/**
 * checkInput - very simple input check
 */
gboolean checkInput(GSList *list){
	__PRINTFUNC__;

	GSList			*next;
	while(list){
		ContactCards_item_t		*item;
		next = list->next;
		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		switch(item->itemID){
			case CARDTYPE_FN_FIRST:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					return FALSE;
				break;
			case CARDTYPE_FN_LAST:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					return FALSE;
				break;
		}
stepForward:
		list = next;
	}
	return TRUE;
}

/**
 * contactEditSave - save the changes on a vCard
 */
static void contactEditSave(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GError						*error = NULL;
	GThread						*thread;
	ContactCards_add_t			*value = trans;

	if(checkInput(value->list)== FALSE){
		feedbackDialog(GTK_MESSAGE_ERROR, _("Unable to save changes"));
	}
	viewCleaner(appBase.contactView);
	thread = g_thread_try_new("pushing vCard", pushingCard, trans, &error);
	if(error){
		verboseCC("[%s] something has gone wrong with threads\n", __func__);
		verboseCC("%s\n", error->message);
	}
	g_thread_unref(thread);

}

/**
 * menuNewPhone - callback to add a new phone number to a contact
 */
static void menuNewPhone(GtkButton *btn, GtkWidget *other){
	__PRINTFUNC__;
	debugCC("It doesn't work so far...\n");
}

/**
 * menuNewMail - callback to add a new Mail to a contact
 */
static void menuNewMail(GtkButton *btn, GtkWidget *other){
	__PRINTFUNC__;
	debugCC("It doesn't work so far...\n");
}

/**
 * menuNewUrl - callback to add a new Url to a contact
 */
static void menuNewUrl(GtkButton *btn, GtkWidget *other){
	__PRINTFUNC__;
	debugCC("It doesn't work so far...\n");
}

/**
 * buildEditCard - display the data of a selected vCard for editing
 */
static GtkWidget *buildEditCard(sqlite3 *ptr, int selID, int abID){
	__PRINTFUNC__;

	GtkWidget				*card, *frame;
	GtkWidget				*saveBtn, *discardBtn, *addItemBtn, *addMenu;
	GtkWidget				*typ, *label, *input, *item;
	GSList					*list, *items;
	GtkEntryBuffer			*prefixBuf, *firstNBuf, *middleNBuf, *lastNBuf, *suffixBuf;
	ContactCards_item_t		*prefixItem, *firstNItem, *middleNItem, *lastNItem, *suffixItem;
	int						line = 0;
	char					*vData = NULL;
	char					*naming;
	gchar					**namingPtr = NULL;
	ContactCards_add_t			*transNew;
	ContactCards_new_Value_t	*transPhone, *transUrl, *transEMail, *transNote;

	frame = gtk_frame_new(NULL);
	card = gtk_grid_new();
	if(selID){
		vData = getSingleChar(ptr, "contacts", "vCard", 1, "contactID", selID, "", "", "", "", "", 0);
			if(vData == NULL)
				return card;
	}

	gtk_widget_set_hexpand(GTK_WIDGET(frame), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(frame), TRUE);
	gtk_widget_set_hexpand(GTK_WIDGET(card), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(card), TRUE);
	gtk_widget_set_halign(GTK_WIDGET(card), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(card), GTK_ALIGN_START);

	items = g_slist_alloc();

	transNew = g_new(ContactCards_add_t, 1);

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

	transPhone = g_new(ContactCards_new_Value_t, 1);
	transUrl = g_new(ContactCards_new_Value_t, 1);
	transEMail = g_new(ContactCards_new_Value_t, 1);
	transNote = g_new(ContactCards_new_Value_t, 1);

	saveBtn = gtk_button_new_with_label(_("Save"));
	discardBtn = gtk_button_new_with_label(_("Discard"));
	gtk_grid_attach(GTK_GRID(card), saveBtn, 4, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), discardBtn, 5, line, 1, 1);


	addItemBtn = gtk_menu_button_new();
	gtk_widget_set_tooltip_text(GTK_WIDGET(addItemBtn), _("Add Information"));
	addMenu = gtk_menu_new();
	item = gtk_menu_item_new_with_label(_("Add Phone"));
	gtk_menu_shell_append(GTK_MENU_SHELL(addMenu), item);
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menuNewPhone),  NULL);
	item = gtk_menu_item_new_with_label(_("Add EMail"));
	gtk_menu_shell_append(GTK_MENU_SHELL(addMenu), item);
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menuNewMail),  NULL);
	item = gtk_menu_item_new_with_label(_("Add URL"));
	gtk_menu_shell_append(GTK_MENU_SHELL(addMenu), item);
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menuNewUrl),  NULL);
	gtk_widget_show_all(addMenu);
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(addItemBtn), addMenu);
	gtk_grid_attach(GTK_GRID(card), addItemBtn, 6, line, 1, 1);
	line++;

	if(selID){
		naming = getSingleCardAttribut(CARDTYPE_N, vData);
		if(naming){
			namingPtr = g_strsplit(naming, ";", 5);
			gtk_entry_buffer_set_text(lastNBuf, g_strstrip(namingPtr[0]), -1);
			gtk_entry_buffer_set_text(firstNBuf, g_strstrip(namingPtr[1]), -1);
			gtk_entry_buffer_set_text(middleNBuf, g_strstrip(namingPtr[2]), -1);
			gtk_entry_buffer_set_text(prefixBuf, g_strstrip(namingPtr[3]), -1);
			gtk_entry_buffer_set_text(suffixBuf, g_strstrip(namingPtr[4]), -1);
			g_strfreev(namingPtr);
			g_free(naming);
		} else {
			gtk_entry_buffer_set_text(lastNBuf, "", 0);
			gtk_entry_buffer_set_text(firstNBuf, "", 0);
			gtk_entry_buffer_set_text(middleNBuf, "", 0);
			gtk_entry_buffer_set_text(prefixBuf, "", 0);
			gtk_entry_buffer_set_text(suffixBuf, "", 0);
		}
	}

	/*		Naming	*/
	typ = gtk_label_new(_("Name"));
	gtk_widget_set_margin_start(typ, 12);
	gtk_widget_set_margin_end(typ, 12);
	gtk_widget_set_margin_top(typ, 18);
	gtk_widget_set_halign(typ, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(card), typ, 1, line, 1, 1);
	line++;

	label = gtk_label_new(_("Prefix"));
	gtk_widget_set_halign(label, GTK_ALIGN_END);
	gtk_widget_set_margin_end(label, 6);
	input = gtk_entry_new_with_buffer(prefixBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 2, line++, 5, 1);
	prefixItem->itemID = CARDTYPE_FN_PREFIX;
	prefixItem->element = prefixBuf;
	items = g_slist_append(items, prefixItem);

	label = gtk_label_new(_("First name"));
	gtk_widget_set_halign(label, GTK_ALIGN_END);
	gtk_widget_set_margin_end(label, 6);
	input = gtk_entry_new_with_buffer(firstNBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 2, line++, 5, 1);
	firstNItem->itemID = CARDTYPE_FN_FIRST;
	firstNItem->element = firstNBuf;
	items = g_slist_append(items, firstNItem);

	label = gtk_label_new(_("Middle name"));
	gtk_widget_set_halign(label, GTK_ALIGN_END);
	gtk_widget_set_margin_end(label, 6);
	input = gtk_entry_new_with_buffer(middleNBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 2, line++, 5, 1);
	middleNItem->itemID = CARDTYPE_FN_MIDDLE;
	middleNItem->element = middleNBuf;
	items = g_slist_append(items, middleNItem);

	label = gtk_label_new(_("Last name"));
	gtk_widget_set_halign(label, GTK_ALIGN_END);
	gtk_widget_set_margin_end(label, 6);
	input = gtk_entry_new_with_buffer(lastNBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 2, line++, 5, 1);
	lastNItem->itemID = CARDTYPE_FN_LAST;
	lastNItem->element = lastNBuf;
	items = g_slist_append(items, lastNItem);

	label = gtk_label_new(_("Suffix"));
	gtk_widget_set_halign(label, GTK_ALIGN_END);
	gtk_widget_set_margin_end(label, 6);
	input = gtk_entry_new_with_buffer(suffixBuf);
	gtk_widget_set_margin_bottom(input, 4);
	gtk_grid_attach(GTK_GRID(card), label, 1, line, 1, 1);
	gtk_grid_attach(GTK_GRID(card), input, 2, line++, 5, 1);
	suffixItem->itemID = CARDTYPE_FN_SUFFIX;
	suffixItem->element = suffixBuf;
	items = g_slist_append(items, suffixItem);

	/*	Phone	*/
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_TEL, vData, FALSE);
		if (g_slist_length(list) > 1){
			typ = gtk_label_new(_("Phone"));
			gtk_widget_set_margin_start(typ, 12);
			gtk_widget_set_margin_end(typ, 12);
			gtk_widget_set_margin_top(typ, 18);
			gtk_widget_set_halign(typ, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
			while(list){
				GSList				*next = list->next;
				char				*value = (char *) list->data;
				if(value != NULL){
					line = contactEditSingleItem(card, items, CARDTYPE_TEL, line, g_strstrip(value));
				}
				list = next;
			}
		}
		g_slist_free_full(list, g_free);
		transPhone->grid = card;
		transPhone->list = items;
		transPhone->type = CARDTYPE_TEL;
		line++;
	}

	/*	EMail	*/
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_EMAIL, vData, FALSE);
		if (g_slist_length(list) > 1){
			typ = gtk_label_new(_("EMail"));
			gtk_widget_set_margin_start(typ, 12);
			gtk_widget_set_margin_end(typ, 12);
			gtk_widget_set_margin_top(typ, 18);
			gtk_widget_set_halign(typ, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
			while(list){
				GSList				*next = list->next;
				char				*value = (char *) list->data;
				if(value != NULL){
					line = contactEditSingleItem(card, items, CARDTYPE_EMAIL, line, g_strstrip(value));
				}
				list = next;
			}
		}
		g_slist_free_full(list, g_free);
		transEMail->grid = card;
		transEMail->list = items;
		transEMail->type = CARDTYPE_EMAIL;
		line++;
	}

	/*	URL	*/
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_URL, vData, FALSE);
		if (g_slist_length(list) > 1){
			typ = gtk_label_new(_("URL"));
			gtk_widget_set_margin_start(typ, 12);
			gtk_widget_set_margin_end(typ, 12);
			gtk_widget_set_margin_top(typ, 18);
			gtk_widget_set_halign(typ, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
			while(list){
				GSList				*next = list->next;
				char				*value = (char *) list->data;
				if(value != NULL){
					line = contactEditSingleItem(card, items, CARDTYPE_URL, line, g_strstrip(value));
				}
				list = next;
			}
		}
		g_slist_free_full(list, g_free);
		transUrl->grid = card;
		transUrl->list = items;
		transUrl->type = CARDTYPE_URL;
		line++;
	}

	/*	Address	*/
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_ADR, vData, FALSE);
		if (g_slist_length(list) > 1){
			typ = gtk_label_new(_("Address"));
			gtk_widget_set_margin_start(typ, 12);
			gtk_widget_set_margin_end(typ, 12);
			gtk_widget_set_margin_top(typ, 18);
			gtk_widget_set_halign(typ, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
			while(list){
				GSList				*next = list->next;
				char				*value = (char *) list->data;
				if(value != NULL){
					line = contactEditPostalItem(card, items, line, g_strstrip(value));
				}
				list = next;
			}
		}
		g_slist_free_full(list, g_free);
		line++;
	}

	/*	Note	*/
	if(selID){
		list = getMultipleCardAttribut(CARDTYPE_NOTE, vData, FALSE);
		if (g_slist_length(list) > 1){
			typ = gtk_label_new(_("Note"));
			gtk_widget_set_margin_start(typ, 12);
			gtk_widget_set_margin_end(typ, 12);
			gtk_widget_set_margin_top(typ, 18);
			gtk_widget_set_halign(typ, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(card), typ, 1, line++, 1, 1);
			while(list){
				GSList				*next = list->next;
				char				*value = (char *) list->data;
				if(value != NULL){
					line = contactEditSingleMultilineItem(card, items, CARDTYPE_NOTE, line, g_strstrip(value));
				}
				list = next;
			}
		}
		g_slist_free_full(list, g_free);
		transNote->grid = card;
		transNote->list = items;
		transNote->type = CARDTYPE_NOTE;;
		line++;
	}

	/*	dirty hack to add some space between the last entry and the end of the containing widget	*/
	item = gtk_label_new("");
	gtk_widget_set_margin_start(GTK_WIDGET(item), 24);
	gtk_widget_set_margin_end(GTK_WIDGET(item), 24);
	gtk_grid_attach(GTK_GRID(card), GTK_WIDGET(item), 1, line++, 5, 1);

	gtk_container_add(GTK_CONTAINER(frame), card);

	transNew->list = items;
	transNew->editID = selID;

	/*		Connect Signales		*/
	g_signal_connect(G_OBJECT(saveBtn), "clicked", G_CALLBACK(contactEditSave), transNew);
	g_signal_connect(G_OBJECT(discardBtn), "clicked", G_CALLBACK(contactEditDiscard), transNew);

	return frame;
}

/**
 * contactNew - add a new vCard
 */
static void contactNew(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GtkWidget			*newCard;
	int					abID = 0,
						selTyp = 0;
	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					srvID	= 0,
						flag	= 0;


	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.addressbookList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COL, &abID, TYP_COL, &selTyp, -1);
	}

	verboseCC("[%s] %d\n",__func__, abID);

	if(abID == 0 || selTyp == 0){
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no address book selected."));
		return;
	}

	srvID = getSingleInt(appBase.db, "addressbooks", "cardServer", 1, "addressbookID", abID, "", "", "", "");
	flag = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", srvID, "", "", "", "");
	if((flag & CONTACTCARDS_ONE_WAY_SYNC) == CONTACTCARDS_ONE_WAY_SYNC){
		feedbackDialog(GTK_MESSAGE_WARNING, _("With One-Way-Sync enabled you are not allowed to do this!"));
		return;
	}

	newCard = buildEditCard(appBase.db, 0, abID);
	viewCleaner(appBase.contactView);
	gtk_widget_show_all(newCard);
	gtk_container_add(GTK_CONTAINER(appBase.contactView), newCard);
}
/**
 * contactEdit - edit the content of a vCard
 */
static void contactEdit(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkWidget			*editCard;
	int					selID;
	int					flag 	= 0,
						abID 	= 0,
						srvID 	= 0;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);
	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no vCard selected to edit."));
		return;
	}

	abID = getSingleInt(appBase.db, "contacts", "addressbookID", 1, "contactID", selID, "", "", "", "");
	srvID = getSingleInt(appBase.db, "addressbooks", "cardServer", 1, "addressbookID", abID, "", "", "", "");
	flag = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", srvID, "", "", "", "");
	if((flag & CONTACTCARDS_ONE_WAY_SYNC) == CONTACTCARDS_ONE_WAY_SYNC){
		feedbackDialog(GTK_MESSAGE_WARNING, _("With One-Way-Sync enabled you are not allowed to do this!"));
		return;
	}

	editCard = buildEditCard(appBase.db, selID, 0);
	viewCleaner(appBase.contactView);
	gtk_widget_set_halign (editCard, GTK_ALIGN_FILL);
	gtk_widget_set_valign (editCard, GTK_ALIGN_FILL);
	gtk_widget_show_all(editCard);
	gtk_container_add(GTK_CONTAINER(appBase.contactView), editCard);

}

/**
 * contactEditdb - Callback for contactEdit()
 */
static void contactEditcb(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	contactEdit(NULL, NULL);
}

/**
 * contactExportcb - Callback to export one Contact
 */
static void contactExportcb(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					selID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);
	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no vCard selected to export."));
		return;
	}

	cbExportContacts(3, selID);

}

/**
 * contactAddFavcb - Callback to add one Contact to Favorites
 */
static void contactAddFavcb(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					selID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);
	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no vCard selected."));
		return;
	}
	dbFlagSet(appBase.db, "contacts", "flags", "contactID", selID, CONTACTCARDS_FAVORIT);
}

/**
 * contactCopycb - Callback to copy one Contact to another address book
 */
static void contactCopycb(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;
	int				destABook = GPOINTER_TO_INT(data);
	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					selID;
	char				*vCard = NULL;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] copy vCard %d to address book %d\n",__func__, selID, destABook);

		vCard = getSingleChar(appBase.db, "contacts", "vCard", 1, "contactID", selID, "", "", "", "","",0);
		pushCard(appBase.db, vCard, destABook, 0, 0);
		g_free(vCard);

	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no vCard selected."));
		return;
	}

}

/**
 * contactDelFavcb - Callback to add one Contact to Favorites
 */
static void contactDelFavcb(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					selID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);
	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no vCard selected."));
		return;
	}
	dbFlagDel(appBase.db, "contacts", "flags", "contactID", selID, CONTACTCARDS_FAVORIT);
}

/**
 * completionContact - select a vCard from the searchbar
 */
static void completionContact(GtkEntryCompletion *widget, GtkTreeModel *model, GtkTreeIter *iter, gpointer trans){
	__PRINTFUNC__;

	GtkWidget					*card;
	int							selID;

	gtk_tree_model_get(model, iter, SELECTION_COLUMN, &selID,  -1);
	verboseCC("[%s] %d\n",__func__, selID);
	gtk_tree_selection_select_iter(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), iter);
	card = buildNewCard(appBase.db, selID);
	gtk_widget_show_all(card);
	viewCleaner(appBase.contactView);
	gtk_container_add(GTK_CONTAINER(appBase.contactView), card);
}

/**
 * selContact - select a vCard from the list
 */
static void selContact(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkWidget			*card;
	int					selID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		debugCC("[%s] %d\n",__func__, selID);
		card = buildNewCard(appBase.db, selID);
		gtk_widget_show_all(card);
		viewCleaner(appBase.contactView);
		gtk_container_add(GTK_CONTAINER(appBase.contactView), card);
	}
}

/**
 * listFlush - remove all items from a list
 */
void listFlush(GtkWidget *list){
	__PRINTFUNC__;

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
	__PRINTFUNC__;

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
	__PRINTFUNC__;

	GtkListStore		*store;
	GtkTreeViewColumn	*column;
	GtkCellRenderer		*renderer;
	GtkTreeSortable		*sortable;


	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "text", TEXT_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_UINT);

	sortable = GTK_TREE_SORTABLE(store);

	gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));

	g_object_unref(store);
}

/**
 * listSortorderAsc - sort a list ascending
 */
static void listSortorderAsc(void){
	__PRINTFUNC__;

	GtkTreeStore		*store;

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.contactList)));

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LAST_COLUMN, GTK_SORT_ASCENDING);

}

/**
 * listSortorderDesc - sort a list descending
 */
static void listSortorderDesc(void){
	__PRINTFUNC__;

	GtkTreeStore		*store;

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.contactList)));

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LAST_COLUMN, GTK_SORT_DESCENDING);

}

/**
 * syncOneServer - check one single server for new data
 */
void *syncOneServer(void *trans){
	__PRINTFUNC__;

	int					serverID = GPOINTER_TO_INT(trans);
	int					isOAuth = 0;
	ne_session 			*sess = NULL;
	char				*srv = NULL;
	char				*msg = NULL;
	int					ctxID;

	srv = getSingleChar(appBase.db, "cardServer", "desc", 1, "serverID", serverID, "", "", "", "", "", 0);
	msg = g_strconcat(_("syncing "), srv, NULL);
	ctxID = gtk_statusbar_get_context_id(GTK_STATUSBAR(appBase.statusbar), "info");
	gtk_statusbar_push(GTK_STATUSBAR(appBase.statusbar), ctxID, msg);

	isOAuth = getSingleInt(appBase.db, "cardServer", "isOAuth", 1, "serverID", serverID, "", "", "", "");

	if(isOAuth){
		int 		ret = 0;
		ret = oAuthUpdate(appBase.db, serverID);
		if(ret != OAUTH_UP2DATE){
			gtk_statusbar_pop(GTK_STATUSBAR(appBase.statusbar), ctxID);
			g_mutex_unlock(&mutex);
            verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
			g_free(srv);
			g_free(msg);
			return NULL;
		}
	}

	sess = serverConnect(serverID);
	syncContacts(appBase.db, sess, serverID);
	serverDisconnect(sess, appBase.db, serverID);

	gtk_statusbar_pop(GTK_STATUSBAR(appBase.statusbar), ctxID);

	g_free(srv);
	g_free(msg);
	g_mutex_unlock(&mutex);
    verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);

	addressbookTreeUpdate();

	return NULL;
}

void *importCards(void *trans){
	__PRINTFUNC__;

	ContactCards_item_t		*data = trans;
	int						aID = data->itemID;
	int						newID = 0;
	GSList					*cards = data->element;

	while(g_mutex_trylock(&mutex) != TRUE){}
    verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);
	while(cards){
		GSList				*next = cards->next;
		if(cards->data == NULL){
			cards = next;
			continue;
		}
		/*
		 * Handle local contacts
		 * They will not leave this database!
		 */
		if(aID == 0){
			newID = newContact(appBase.db, 0, g_strstrip(cards->data));
			setSingleInt(appBase.db, "contacts", "flags", CONTACTCARDS_LOCAL, "contactID", newID);
		} else {
			pushCard(appBase.db, g_strstrip(cards->data), aID, 0, 0);
		}
		cards = next;
	}
	g_mutex_unlock(&mutex);
    verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
	g_slist_free_full(cards, g_free);
	g_free(data);

	return NULL;
}

/**
 * importVCF - context menu callback to import vCards
 */
static void importVCF(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	GtkWidget			*chooser;
	GError				*error = NULL;
	GSList				*cards;
	char				*filename=NULL;
	char				*content = NULL;
	int					aID = 0;
	int					result = 0;
	GThread						*thread;
	ContactCards_item_t			*trans = NULL;

	trans = g_new(ContactCards_item_t, 1);

	aID = GPOINTER_TO_INT(data);

	debugCC("[%s] aID: %d\n", __func__, aID);

	chooser = gtk_file_chooser_dialog_new (_("Open *.vcf"),
											GTK_WINDOW (appBase.window),
											GTK_FILE_CHOOSER_ACTION_OPEN,
											_("_Cancel"),
											GTK_RESPONSE_CANCEL,
											_("_Import"),
											GTK_RESPONSE_OK,
											NULL);

	result = gtk_dialog_run (GTK_DIALOG (chooser));

	switch(result){
		case GTK_RESPONSE_OK:
			filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
			break;
		default:
			break;
	}

	gtk_widget_destroy (chooser);

	if(filename == NULL)
		return;

	g_file_get_contents(filename, &content, NULL, &error);
	if(error){
		verboseCC("[%s] %s\n", __func__, error->message);
		g_free(filename);
		g_free(content);
		return;
	}

	cards = validateFile(content);

	trans->itemID = aID;
	trans->element = cards;

	thread = g_thread_try_new("syncingServer", importCards, trans, &error);
	if(error){
		verboseCC("[%s] something has gone wrong with threads\n", __func__);
		verboseCC("%s\n", error->message);
	}
	g_thread_unref(thread);

	g_free(filename);
	g_free(content);
	return;
}

/**
 * addressbookDel - delete an address book from the server
 */
static void addressbookDel(GtkMenuItem *menuitem, gpointer data){
	__PRINTFUNC__;

	GtkWidget			*dialog;
	int					srvID, aID;
	ne_session 			*sess = NULL;
	int 				resp;
	int					isOAuth = 0;
	int					flag = 0;

	aID = GPOINTER_TO_INT(data);

	srvID = getSingleInt(appBase.db, "addressbooks", "cardServer", 1, "addressbookID", aID, "", "", "", "");

	flag = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", srvID, "", "", "", "");
	if((flag & CONTACTCARDS_ONE_WAY_SYNC) == CONTACTCARDS_ONE_WAY_SYNC){
		feedbackDialog(GTK_MESSAGE_WARNING, _("With One-Way-Sync enabled you are not allowed to do this!"));
		return;
	}

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT & GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Do you really want to delete this address book?"));

	debugCC("[%s] deleting address book %d\n", __func__, aID);

	resp = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if (resp != GTK_RESPONSE_YES) return;

	while(g_mutex_trylock(&mutex) != TRUE){}
    verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);

	isOAuth = getSingleInt(appBase.db, "cardServer", "isOAuth", 1, "serverID", srvID, "", "", "", "");

	if(isOAuth){
		int 		ret = 0;
		ret = oAuthUpdate(appBase.db, srvID);
		if(ret != OAUTH_UP2DATE)
			g_mutex_unlock(&mutex);
			return;
	}

	sess = serverConnect(srvID);
	serverDelCollection(appBase.db, sess, srvID, aID);
	serverDisconnect(sess, appBase.db, srvID);
	addressbookTreeUpdate();

	g_mutex_unlock(&mutex);
    verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
}

/**
 * addressbookTreeContextMenu - a simple context menu for the address books view
 */
void addressbookTreeContextMenu(GtkWidget *widget, GdkEvent *event, gpointer data){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					selID;
	int					typ;
	int 				flags = 0;
	int					srvID = 0;

	/*	right mouse button	*/
	if(event->button.button != 3)
		return;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.addressbookList))), &model, &iter)) {
		GtkWidget			*menu = NULL,
							*menuItem = NULL,
							*menuItem2 = NULL,
							*menuItem3 = NULL,
							*menuItem4 = NULL,
							*menuItem5 = NULL;
		gtk_tree_model_get(model, &iter, TYP_COL, &typ, ID_COL, &selID,  -1);
		verboseCC("[%s] typ: %d\tselID: %d\n",__func__, typ, selID);

		menu = gtk_menu_new();

		switch(typ){
			case 0:		/*	server	*/
				menuItem3 = gtk_menu_item_new_with_label(_("Export Contacts"));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem3);
				if(selID == 0){
					g_signal_connect(menuItem3, "activate", (GCallback)cbExportContactAll, GINT_TO_POINTER(selID));
					break;
				} else {
					g_signal_connect(menuItem3, "activate", (GCallback)cbExportContactSrv, GINT_TO_POINTER(selID));
				}
				menuItem2 = gtk_menu_item_new_with_label(_("Export Birthdays"));
				g_signal_connect(menuItem2, "activate", (GCallback)cbSrvExportBirthdays, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem2);
				flags = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", selID, "", "", "", "");
				if((flags & DAV_OPT_MKCOL) == DAV_OPT_MKCOL){
					verboseCC("[%s] %d supports MKCOL\n", __func__, selID);
				} else {
					verboseCC("[%s] %d doesn't support MKCOL\n", __func__, selID);
					break;
				}
				if((flags & CONTACTCARDS_ONE_WAY_SYNC) == CONTACTCARDS_ONE_WAY_SYNC){
					break;
				}
				verboseCC("[%s] Server %d selected\n", __func__, selID);
				menuItem = gtk_menu_item_new_with_label(_("Create new address book"));
				g_signal_connect(menuItem, "activate", (GCallback)createNewCollection, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
				menuItem4 = gtk_menu_item_new_with_label(_("Sync with server"));
				g_signal_connect(menuItem4, "activate", (GCallback)syncMenuSel, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem4);
				break;
			case 1:		/* address book	*/
				verboseCC("[%s] Adress book %d selected\n", __func__, selID);
				srvID = getSingleInt(appBase.db, "addressbooks", "cardServer", 1, "addressbookID", selID, "", "", "", "");
				flags = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", srvID, "", "", "", "");
				menuItem4 = gtk_menu_item_new_with_label(_("Export Birthdays"));
				g_signal_connect(menuItem4, "activate", (GCallback)cbAddrBookExportBirthdays, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem4);
				menuItem5 = gtk_menu_item_new_with_label(_("Export Contacts"));
				g_signal_connect(menuItem5, "activate", (GCallback)cbExportContactABook, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem5);
				if((flags & CONTACTCARDS_ONE_WAY_SYNC) == CONTACTCARDS_ONE_WAY_SYNC){
					break;
				}
				menuItem = gtk_menu_item_new_with_label(_("Delete address book"));
				g_signal_connect(menuItem, "activate", (GCallback)addressbookDel, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
				menuItem2 = gtk_menu_item_new_with_label(_("Add new contact"));
				g_signal_connect(menuItem2, "activate", (GCallback)contactNew, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem2);
				menuItem3 = gtk_menu_item_new_with_label(_("Import *.vcf"));
				g_signal_connect(menuItem3, "activate", (GCallback)importVCF, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem3);
				break;
			case 2:		/*	favorites	*/
				menuItem2 = gtk_menu_item_new_with_label(_("Export Birthdays"));
				g_signal_connect(menuItem2, "activate", (GCallback)cbFavoritesExportBirthdays, NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem2);
				menuItem5 = gtk_menu_item_new_with_label(_("Export Contacts"));
				g_signal_connect(menuItem5, "activate", (GCallback)cbExportContactFav, NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem5);
				break;
			case 3:		/*	locales		*/
				menuItem2 = gtk_menu_item_new_with_label(_("Export Birthdays"));
				g_signal_connect(menuItem2, "activate", (GCallback)cbLocalesExportBirthdays, NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem2);
				menuItem5 = gtk_menu_item_new_with_label(_("Export Contacts"));
				g_signal_connect(menuItem5, "activate", (GCallback)cbExportContactLoc, NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem5);
				menuItem3 = gtk_menu_item_new_with_label(_("Import *.vcf"));
				g_signal_connect(menuItem3, "activate", (GCallback)importVCF, GINT_TO_POINTER(0));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem3);
			default:
				break;
		}
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button.button, gdk_event_get_time((GdkEvent*)event));
	}
}

/**
 * addressbookTreeUpdate - updates the address books view
 */
void addressbookTreeUpdate(void){
	__PRINTFUNC__;

	GtkTreeStore	*store;
	GSList			*servers, *addressBooks;
	GtkTreeIter		toplevel, child;
	GtkTreeSelection	*selection;

	while(g_mutex_trylock(&aBookTreeMutex) != TRUE){}
    verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);
	/* Flush the tree	*/
	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.addressbookList)));
	/*	something is wrong	*/
	if(store == NULL){
		g_mutex_unlock(&aBookTreeMutex);
        verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
		return;
	}

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (appBase.addressbookList));
	gtk_tree_selection_unselect_all(selection);

	gtk_tree_store_clear(store);

	/* Insert new elements	*/
	servers = getListInt(appBase.db, "cardServer", "serverID", 0, "", 0, "", "", "", "");
	gtk_tree_store_append(store, &toplevel, NULL);
	gtk_tree_store_set(store, &toplevel, DESC_COL, _("All"), ID_COL, 0, TYP_COL, 0,  -1);
	gtk_tree_selection_select_iter(selection, &toplevel);

	gtk_tree_store_append(store, &toplevel, NULL);
	gtk_tree_store_set(store, &toplevel, DESC_COL, _("Favorites"), ID_COL, 0, TYP_COL, 2,  -1);

	if((appBase.flags & CONTACTCARDS_NO_LOCAL) != CONTACTCARDS_NO_LOCAL){
		gtk_tree_store_append(store, &toplevel, NULL);
		gtk_tree_store_set(store, &toplevel, DESC_COL, _("Locales"), ID_COL, 0, TYP_COL, 3,  -1);
	}

	if(g_slist_length(servers) <= 1){
		debugCC("There are no servers actually\n");
		g_mutex_unlock(&aBookTreeMutex);
        verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
		return;
	}
	while(servers){
		GSList				*next = servers->next;
		int					serverID = GPOINTER_TO_INT(servers->data);
		char				*serverDesc = NULL;
		if(serverID == 0){
			servers = next;
			continue;
		}
		serverDesc = getSingleChar(appBase.db, "cardServer", "desc", 1, "serverID", serverID, "", "", "", "", "", 0);
		gtk_tree_store_append(store, &toplevel, NULL);
		gtk_tree_store_set(store, &toplevel, DESC_COL, serverDesc, ID_COL, serverID, TYP_COL, 0,  -1);

		addressBooks = getListInt(appBase.db, "addressbooks", "addressbookID", 1, "cardServer", serverID, "", "", "", "");

		if(g_slist_length(addressBooks) <= 1){
			debugCC("There are no address books actually\n");
			g_slist_free(addressBooks);
			g_free(serverDesc);
			servers = next;
			continue;
		}

		while(addressBooks){
			GSList				*next2 =  addressBooks->next;
			int					addressbookID = GPOINTER_TO_INT(addressBooks->data);
			char				*displayname = NULL;
			int					active = 0;
			if(addressbookID == 0){
				addressBooks = next2;
				continue;
			}
			active = getSingleInt(appBase.db, "addressbooks", "syncMethod", 1, "addressbookID", addressbookID, "", "", "", "");
			if(active & (1<<DAV_ADDRBOOK_DONT_SYNC)){
				/* Don't display address books which are not synced	*/
				addressBooks = next2;
				continue;
			}
			displayname = getSingleChar(appBase.db, "addressbooks", "displayname", 1, "addressbookID", addressbookID, "", "", "", "", "", 0);
			gtk_tree_store_append(store, &child, &toplevel);
			gtk_tree_store_set(store, &child, DESC_COL, displayname, ID_COL, addressbookID, TYP_COL, 1,  -1);

			g_free(displayname);
			addressBooks = next2;
		}
		g_slist_free(addressBooks);
		g_free(serverDesc);
		servers = next;
	}
	g_slist_free(servers);
	g_mutex_unlock(&aBookTreeMutex);
    verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
	calendarUpdate(0,0);
}

/**
 * addressbookModelCreate - create the model for the address books view
 */
static GtkTreeModel *addressbookModelCreate(void){
	__PRINTFUNC__;

	GtkTreeStore  *treestore;

	treestore = gtk_tree_store_new(TOTAL_COLS, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);

	return GTK_TREE_MODEL(treestore);
}

gboolean addressbookTreeCursorCB (GtkTreeView *view, GtkMovementStep step, gint dir, gpointer data){
	__PRINTFUNC__;

	GtkTreeModel			*model;
	GtkTreeIter				iter;
	GtkTreePath				*path;

	if(step != GTK_MOVEMENT_VISUAL_POSITIONS){
		return FALSE;
	}

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(view), &model, &iter))
	{
		path = gtk_tree_model_get_path(model, &iter);
		if(gtk_tree_view_row_expanded(view, path) == TRUE){
			gtk_tree_view_collapse_row(view, path);
		} else {
			gtk_tree_view_expand_to_path(view, path);
		}
	}

	return TRUE;
}

/**
 * addressbookTreeCreate - creates the model and view for the adress books
 */
GtkWidget *addressbookTreeCreate(void){
	__PRINTFUNC__;

	GtkWidget				*view;
	GtkTreeViewColumn		*column;
	GtkTreeModel			*model;
	GtkCellRenderer			*renderer;

	view = gtk_tree_view_new();

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "text", DESC_COL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

	model = addressbookModelCreate();
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_SINGLE);

	g_signal_connect(G_OBJECT(view), "move-cursor", G_CALLBACK(addressbookTreeCursorCB), NULL);

	return view;
}

/**
 * contactsTreeContextMenu - a simple context menu for the list view
 */
void contactsTreeContextMenu(GtkWidget *widget, GdkEvent *event, gpointer data){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					selID;
	int					abID	= 0,
						srvID	= 0,
						flag	= 0,
						userflag = 0;

	/*	right mouse button	*/
	if(event->button.button != 3)
		return;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		GtkWidget		*menu,
						*delItem,
						*editItem,
						*exportItem,
						*favItem;
		GSList			*server, *aBooks;
		GtkWidget		*cpyMenu, *subMenu;
		int				cpyDst = 0;
		int				serverflag = 0;
		int				serverID = 0;
		int				aBookID = 0;

		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);

		menu = gtk_menu_new();
		abID = getSingleInt(appBase.db, "contacts", "addressbookID", 1, "contactID", selID, "", "", "", "");
		srvID = getSingleInt(appBase.db, "addressbooks", "cardServer", 1, "addressbookID", abID, "", "", "", "");
		flag = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", srvID, "", "", "", "");
		if((flag & ~CONTACTCARDS_ONE_WAY_SYNC) == flag){
			delItem = gtk_menu_item_new_with_label(_("Delete"));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), delItem);
			g_signal_connect(delItem, "activate", (GCallback)contactDelcb, NULL);
			editItem = gtk_menu_item_new_with_label(_("Edit"));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), editItem);
			g_signal_connect(editItem, "activate", (GCallback)contactEditcb, NULL);
		}
		exportItem = gtk_menu_item_new_with_label(_("Export"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), exportItem);
		g_signal_connect(exportItem, "activate", (GCallback)contactExportcb, NULL);
		userflag = getSingleInt(appBase.db, "contacts", "flags", 1, "contactID", selID, "", "", "", "");
		if((userflag & CONTACTCARDS_FAVORIT) == CONTACTCARDS_FAVORIT){
			favItem = gtk_menu_item_new_with_label(_("Remove from Favorits"));
			g_signal_connect(favItem, "activate", (GCallback)contactDelFavcb, NULL);
		} else {
			favItem = gtk_menu_item_new_with_label(_("Add to Favorits"));
			g_signal_connect(favItem, "activate", (GCallback)contactAddFavcb, NULL);
		}
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), favItem);

		server = getListInt(appBase.db, "cardServer", "serverID", 0, "", 0, "", "", "", "");
		if(g_slist_length(server) <= 1){
			debugCC("There are no servers actually\n");
		} else {
			cpyMenu = gtk_menu_new();
			while(server){
				GSList				*next = server->next;
				char				*srvDesc;

				serverID = GPOINTER_TO_INT(server->data);
				serverflag = 0;
				serverflag = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", serverID, "", "", "", "");
				if((serverflag & CONTACTCARDS_ONE_WAY_SYNC) == CONTACTCARDS_ONE_WAY_SYNC){
					debugCC("[%s] server %d is on ONE_WAY_SYNC\n", __func__, serverID);
					server = next;
					continue;
				}
				aBooks = getListInt(appBase.db, "addressbooks", "addressbookID", 1, "cardServer", serverID, "", "", "", "");

				if(g_slist_length(aBooks) <= 1){
					g_slist_free(aBooks);
					server = next;
					continue;
				}
				srvDesc = getSingleChar(appBase.db, "cardServer", "desc", 1, "serverID", serverID, "", "", "", "", "", 0);

				while(aBooks){
					GtkWidget		*cpyItem;
					GSList			*next2 = aBooks->next;
					char			*aBookDesc = NULL;
					char			*desc = NULL;

					aBookID = GPOINTER_TO_INT(aBooks->data);

					if(aBookID == 0){
						aBooks = next2;
						continue;
					}
					debugCC("[%s] appending address book %d to submenu\n", __func__, aBookID);
					cpyDst++;
					aBookDesc = getSingleChar(appBase.db, "addressbooks", "displayname", 1, "addressbookID", aBookID, "", "", "", "", "", 0);
					desc = g_strconcat(srvDesc, " | ", aBookDesc, NULL);
					cpyItem = gtk_menu_item_new_with_label(desc);
					gtk_menu_shell_append(GTK_MENU_SHELL(cpyMenu), cpyItem);
					g_signal_connect(cpyItem, "activate", (GCallback)contactCopycb, GINT_TO_POINTER(aBookID));
					g_free(desc);
					g_free(aBookDesc);
					aBooks = next2;
				}
				g_slist_free(aBooks);
				g_free(srvDesc);
				server = next;
			}
			if(cpyDst > 0){
				subMenu = gtk_menu_item_new_with_label(_("Copy to ..."));
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(subMenu), cpyMenu);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), subMenu);
			}
		}
		g_slist_free(server);

		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button.button, gdk_event_get_time((GdkEvent*)event));
	}
}

/**
 * contactsTreeAppend - append a new item to a contacts list view
 */
void contactsTreeAppend(char *card, int id){
	__PRINTFUNC__;

	GtkTreeStore		*store;
	GtkTreeIter 		iter;
	GtkTreeSelection	*selection;
	char				*show = NULL,
						*n = NULL,
						*first  = NULL,
						*last = NULL;
	char				**nPtr = NULL;

	n = getSingleCardAttribut(CARDTYPE_N, card);

	if(n){
		nPtr = g_strsplit(n, ";", 5);
		last = g_strndup(g_strstrip(nPtr[0]), strlen(g_strstrip(nPtr[0])));
		first = g_strndup(g_strstrip(nPtr[1]), strlen(g_strstrip(nPtr[1])));
		g_strfreev(nPtr);
	} else {
		last = getSingleCardAttribut(CARDTYPE_FN, card);
		first = g_strndup(" ", sizeof(" "));
	}

	if(strlen(g_strstrip(last)) == 0)
		last = g_strndup("(no name)", sizeof("(no name)"));

	if((appBase.flags & FAMILYNAME_FIRST) == FAMILYNAME_FIRST){
		show = g_strconcat(g_strstrip(last), " ", g_strstrip(first), NULL);
	} else if ((appBase.flags & GIVENNAME_FIRST) == GIVENNAME_FIRST){
		show = g_strconcat(g_strstrip(first), " ", g_strstrip(last), NULL);
	} else if ((appBase.flags & FAMILYNAME_ONLY) == FAMILYNAME_ONLY){
		show = g_strconcat(g_strstrip(last), " ", g_strndup(g_strstrip(first), 1), ".", NULL);
	} else {
		show = g_strconcat(g_strstrip(last), " ", g_strstrip(first), NULL);
	}
	debugCC("[%s] first: %s\tlast: %s\n", __func__, first, last);

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.contactList)));
	if(store == NULL){
		debugCC("[%s] there is no store\n");
		return;
	}

	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, FN_COLUMN, show, FIRST_COLUMN, first, LAST_COLUMN, last, SELECTION_COLUMN, id, SEP_COLUMN, FALSE, -1);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (appBase.contactList));
	gtk_tree_selection_select_iter(selection, &iter);

	g_free(show);
	g_free(n);
	g_free(first);
	g_free(last);
}

/**
 * contactsTreeUpdate - updates the contacts list view
 */
void contactsTreeUpdate(int type, int id){
	__PRINTFUNC__;

	GtkTreeStore	*store;
	GSList			*contacts = NULL;
	GtkTreeSelection	*selection;

	while(g_mutex_trylock(&contactsTreeMutex) != TRUE){}
    verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);
	/* Flush the tree	*/
	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.contactList)));
	/*	something is wrong	*/
	if(store == NULL){
		g_mutex_unlock(&contactsTreeMutex);
        verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
		return;
	}

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (appBase.contactList));
	gtk_tree_selection_unselect_all(selection);

	gtk_tree_store_clear(store);
	/* Insert new elements	*/
	switch(type){
		case 0:		/*	server selected	*/
			if(id == 0){
				contacts = getListInt(appBase.db, "contacts", "contactID", 0, "", 0, "", "", "", "");
				break;
			} else {
				GSList			*addressBooks;
				addressBooks = getListInt(appBase.db, "addressbooks", "addressbookID", 1, "cardServer", id, "", "", "", "");

				while(addressBooks){
					GSList				*next =  addressBooks->next;
					int					addressbookID = GPOINTER_TO_INT(addressBooks->data);
					int					active = 0;
					if(addressbookID == 0){
						addressBooks = next;
						continue;
					}
					active = getSingleInt(appBase.db, "addressbooks", "syncMethod", 1, "addressbookID", addressbookID, "", "", "", "");
					if(active & (1<<DAV_ADDRBOOK_DONT_SYNC)){
						/* Ignore address books which are not synced	*/
						addressBooks = next;
						continue;
					}
					contacts = getListInt(appBase.db, "contacts", "contactID", 1, "addressbookID", addressbookID, "", "", "", "");
					if(g_slist_length(contacts) <= 1){
						debugCC("There are no contacts actually\n");
						g_slist_free(contacts);
					} else {
						contactsTreeFill(contacts);
						g_slist_free(contacts);
					}
					addressBooks = next;
				}
				g_slist_free(addressBooks);
/*
				if((appBase.flags & USE_SEPARATOR) == USE_SEPARATOR)
					contactsTreeSetSeperators();
*/
				g_mutex_unlock(&contactsTreeMutex);
                verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
				return;
			}
			break;
		case 1:		/*	address book selected	*/
			contacts = getListInt(appBase.db, "contacts", "contactID", 1, "addressbookID", id, "", "", "", "");
			break;
		case 2:
			contacts = getListInt(appBase.db, "contacts", "contactID", 91, "flags", CONTACTCARDS_FAVORIT, "", "", "", "");
			break;
		case 3:
			contacts = getListInt(appBase.db, "contacts", "contactID", 91, "flags", CONTACTCARDS_LOCAL, "", "", "", "");
			break;
		default:
			break;
	}
	if(g_slist_length(contacts) <= 1){
		debugCC("There are no contacts actually\n");
		g_slist_free(contacts);
/*
		if((appBase.flags & USE_SEPARATOR) == USE_SEPARATOR)
			contactsTreeSetSeperators();
*/
		g_mutex_unlock(&contactsTreeMutex);
        verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
	} else {
		contactsTreeFill(contacts);
		g_slist_free(contacts);
/*
		if((appBase.flags & USE_SEPARATOR) == USE_SEPARATOR)
			contactsTreeSetSeperators();
*/
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LAST_COLUMN, GTK_SORT_ASCENDING);

		g_mutex_unlock(&contactsTreeMutex);
        verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
	}
}

/**
 * contactsModelCreate - create the model for the contacts list view
 */
static GtkTreeModel *contactsModelCreate(void){
	__PRINTFUNC__;

	GtkTreeStore  *treestore;

	treestore = gtk_tree_store_new(TOTAL_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_BOOLEAN);

	return GTK_TREE_MODEL(treestore);
}

/**
 * contactsTreeSetSeperators - sets the seperators between the contacts
 */
/*
void contactsTreeSetSeperators(void){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeIter			new;
	GtkTreeModel		*model = gtk_tree_view_get_model(GTK_TREE_VIEW(appBase.contactList));
	GtkTreeStore		*store = GTK_TREE_STORE(model);
	char				*cur = NULL,
						*next = NULL;
	int					selID = 0;

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), LAST_COLUMN, GTK_SORT_ASCENDING);

	if(gtk_tree_model_get_iter_first( model, &iter) == FALSE){
		verboseCC("[%s] empty tree\n", __func__);
		return;
	}
	gtk_tree_model_get( model, &iter, LAST_COLUMN, &cur, -1);

	while(gtk_tree_model_iter_next( model, &iter) == TRUE){
		char			*cmpCur = NULL,
						*cmpNext = NULL;

		gtk_tree_model_get( model, &iter, SELECTION_COLUMN, &selID,  LAST_COLUMN, &next, -1);
		if(!next || selID == 0){
			continue;
		}

		cmpCur = g_utf8_strdown(cur, -1);
		cmpNext = g_utf8_strdown(next, -1);

		if(cmpCur[0] != cmpNext[0]){
			// It's quite dirty. Isn't it?
			cmpCur = g_strconcat(cmpCur, "z", NULL);
			gtk_tree_store_append(store, &new, NULL);
			gtk_tree_store_set(store, &new, FN_COLUMN, "", FIRST_COLUMN, "", LAST_COLUMN, cmpCur, SELECTION_COLUMN, 0, SEP_COLUMN, TRUE, -1);
		}

		g_free(cur);
		cur = g_strndup(next, strlen(next));
		g_free(cmpCur);
		g_free(cmpNext);
	}

	if(cur)
		g_free(cur);
	if(next)
		g_free(next);
}
*/

/**
 * contactsTreeSeparator - returns TRUE, if row is a seperator
 */
/*
static gboolean contactsTreeSeparator (GtkTreeModel *model, GtkTreeIter *iter, gpointer data){
	__PRINTFUNC__;

	gboolean		ret = FALSE;

	while(g_mutex_trylock(&contactsTreeMutex) != TRUE){}
	gtk_tree_model_get (model, iter, SEP_COLUMN, &ret, -1);
	g_mutex_unlock(&contactsTreeMutex);

	return ret;
}
*/

/**
 * contactsTreeColor - give the cells of the contacts tree some color
 */
void contactsTreeColor(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data){
	__PRINTFUNC__;

	int			cId = 0,
				aId = 0,
				sId = 0;
	char		*dbColor = NULL;
	GdkRGBA		rgba;

	gtk_tree_model_get(model, iter, SELECTION_COLUMN, &cId,-1);
	aId = getSingleInt(appBase.db, "contacts", "addressbookID", 1, "contactID", cId, "", "", "", "");
	sId = getSingleInt(appBase.db, "addressbooks", "cardServer", 1, "addressbookID", aId, "", "", "", "");

	dbColor = getSingleChar(appBase.db, "cardServer", "color", 1, "serverID", sId, "", "", "", "", "", 0);

	if(gdk_rgba_parse(&rgba, dbColor) == TRUE)
		g_object_set(renderer, "cell-background-rgba", &rgba, NULL);

	g_free(dbColor);
}

/**
 * contactsTreeCreate - creates the model and view for the contacts list
 */
static GtkWidget *contactsTreeCreate(void){
	__PRINTFUNC__;

	GtkWidget				*view;
	GtkTreeViewColumn		*column, *column2;
	GtkTreeModel			*model;
	GtkCellRenderer			*renderer, *renderer2;

	view = gtk_tree_view_new();

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "text", DESC_COL, NULL);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);


	column2 = gtk_tree_view_column_new();
	gtk_tree_view_column_set_fixed_width (column2, 5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column2);
	renderer2 = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column2, renderer2, TRUE);

	gtk_tree_view_column_set_cell_data_func(column2, renderer2, contactsTreeColor, NULL, NULL);

/*
	gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW(view), contactsTreeSeparator, NULL, NULL);
*/

	model = contactsModelCreate();
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_SINGLE);

	return view;
}

/**
 * completionCB - very simple Callback to check whether a entry fits
 */
static gboolean completionCB(GtkEntryCompletion *completion, const gchar *key, GtkTreeIter *iter, gpointer user_data){
	__PRINTFUNC__;

	gboolean			match = FALSE;
	GtkTreeModel		*model;
	char				*first = NULL,
						*last = NULL;
	char				*cFirst = NULL,
						*cLast = NULL,
						*cKey = NULL;
	int					len = 0;

	model = gtk_entry_completion_get_model(completion);
	gtk_tree_model_get(model, iter, FIRST_COLUMN, &first, LAST_COLUMN, &last, -1);

	if(key)
		cKey = g_utf8_casefold(key, -1);
	if(!cKey)
		goto next;
	if(first)
		cFirst = g_utf8_casefold(first, -1);
	if(last)
		cLast = g_utf8_casefold(last, -1);

	len = strlen(cKey);

	if(cFirst){
		if (!strncmp(cKey, cFirst, len)){
			match = TRUE;
			goto next;
		}
	}

	if(cLast){
		if (!strncmp(cKey, cLast, len)){
			match = TRUE;
			goto next;
		}
	}

next:
	if(first)
		g_free(first);
	if(last)
		g_free(last);
	if(cKey)
		g_free(cKey);
	if(cFirst)
		g_free(cFirst);
	if(cLast)
		g_free(cLast);

	return match;
}

/**
 * syncServer - check all available server for new data
 */
void syncServer(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GSList						*retList;
	GError		 				*error = NULL;
	GThread						*thread;

	retList = getListInt(appBase.db, "cardServer", "serverID", 0, "", 0, "", "", "", "");

	if(g_slist_length(retList) <= 1){
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no server to sync."));
		g_slist_free(retList);
		return;
	}

	while(retList){
		GSList				*next = retList->next;
		int					serverID = GPOINTER_TO_INT(retList->data);
		if(serverID == 0){
			retList = next;
			continue;
		}
		while(g_mutex_trylock(&mutex) != TRUE){ }
        verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);
		thread = g_thread_try_new("syncingServer", syncOneServer, GINT_TO_POINTER(serverID), &error);
		if(error){
			verboseCC("[%s] something has gone wrong with threads\n", __func__);
			verboseCC("%s\n", error->message);
			g_mutex_unlock(&mutex);
            verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
		}
		g_thread_unref(thread);
		retList = next;
	}
	g_slist_free(retList);
}

/**
 * guiInit - build the basic GUI
 */
void guiInit(void){
	__PRINTFUNC__;

	GtkWidget			*mainVBox, *mainToolbar, *mainStatusbar, *mainContent;
	GtkWidget			*addressbookWindow;
	GtkWidget			*cal, *leftBox;
	GtkRequisition 		natSize;
	GtkWidget			*contactBox, *contactWindow, *scroll;
	GtkWidget			*addContact, *delContact, *contactButtons, *contactsEdit, *editContact;
	GtkWidget			*ascContact, *descContact, *searchbar;
	GtkWidget			*emptyCard, *noContact;
	GtkWidget			*syncMenu;
	GtkToolItem			*prefItem, *aboutItem, *sep, *newServer, *syncItem;
	GtkTreeSelection	*bookSel, *contactSel;
	GSList 				*cleanUpList = g_slist_alloc();
	GSList				*callist = g_slist_alloc();
	GtkEntryCompletion	*completion;
	GError				*error = NULL;
	GdkPixbuf			*pixbuf;

	gtk_init(NULL, NULL);

	appBase.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(appBase.window), "ContactCards");
	gtk_window_set_default_size(GTK_WINDOW(appBase.window), 902,610);
	pixbuf = gdk_pixbuf_new_from_file("artwork/icon_48.png", &error);
	if(error){
		verboseCC("[%s] something has gone wrong\n", __func__);
		verboseCC("%s\n", error->message);
	}
	gtk_window_set_icon(GTK_WINDOW(appBase.window), pixbuf);
	g_object_unref(pixbuf);

	mainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	/*		Toolbar					*/
	mainToolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(mainToolbar), GTK_TOOLBAR_ICONS);

	newServer = gtk_tool_button_new(NULL, _("New"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(newServer), _("New"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (newServer), "address-book-new");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), newServer, -1);

	prefItem = gtk_tool_button_new(NULL, _("Preferences"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(prefItem), _("Preferences"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (prefItem), "preferences-system");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), prefItem, -1);

	syncItem = gtk_menu_tool_button_new(NULL, _("Refresh"));
	syncMenu = gtk_menu_new();
	appBase.syncMenu = syncMenu;
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(syncItem), syncMenu);
	gtk_widget_set_tooltip_text(GTK_WIDGET(syncItem), _("Refresh"));
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (syncItem), "view-refresh");
	gtk_toolbar_insert(GTK_TOOLBAR(mainToolbar), syncItem, -1);

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

	/*	Sync Menu					*/
	syncMenuUpdate();

	/*		mainContent				*/
	mainContent = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	/*		Addressbookstuff		*/
	addressbookWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(addressbookWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(addressbookWindow, 144, -1);
	gtk_widget_set_vexpand(GTK_WIDGET(addressbookWindow), TRUE);
	appBase.addressbookList = addressbookTreeCreate();

	cal = gtk_calendar_new();
	gtk_calendar_set_display_options(GTK_CALENDAR(cal), GTK_CALENDAR_SHOW_HEADING|GTK_CALENDAR_SHOW_DAY_NAMES);
	gtk_widget_show(cal);
	gtk_widget_get_preferred_size(cal, NULL, &natSize);
	gtk_widget_set_size_request(cal, natSize.width,natSize.height);
	appBase.cal = cal;
	appBase.callist = callist;
	gtk_calendar_set_detail_func(GTK_CALENDAR(appBase.cal), birthdayTooltip, NULL, NULL);

	leftBox = GTK_WIDGET(gtk_box_new(GTK_ORIENTATION_VERTICAL, 2));

	/*		Contactstuff			*/
	contactBox = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	contactWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(contactWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_kinetic_scrolling(GTK_SCROLLED_WINDOW(contactWindow), TRUE);
	gtk_widget_set_size_request(contactWindow, 199, -1);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	emptyCard = gtk_grid_new();
	gtk_widget_set_halign(GTK_WIDGET(emptyCard), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(emptyCard), GTK_ALIGN_CENTER);
	noContact = gtk_image_new_from_icon_name("avatar-default-symbolic", GTK_ICON_SIZE_DIALOG);
	gtk_container_add(GTK_CONTAINER(emptyCard), noContact);
	gtk_container_add(GTK_CONTAINER(scroll), emptyCard);
	appBase.contactList = contactsTreeCreate();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(appBase.contactList), FALSE);
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
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(appBase.contactList))));
	gtk_entry_completion_set_text_column(completion, FN_COLUMN);
	gtk_entry_completion_set_minimum_key_length(completion, 3);
	gtk_entry_completion_set_match_func (completion, completionCB, NULL, NULL);
	searchbar = gtk_entry_new();
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(searchbar), GTK_ENTRY_ICON_SECONDARY, "stock_search");
	gtk_entry_set_completion(GTK_ENTRY(searchbar), GTK_ENTRY_COMPLETION(completion));
	gtk_widget_set_vexpand(GTK_WIDGET(appBase.contactList), TRUE);
	gtk_container_add(GTK_CONTAINER(contactsEdit), contactWindow);
	gtk_container_add(GTK_CONTAINER(contactsEdit), searchbar);
	gtk_container_add(GTK_CONTAINER(contactsEdit), contactButtons);

	gtk_tree_view_set_search_column(GTK_TREE_VIEW(appBase.contactList), LAST_COLUMN);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(appBase.contactList), TRUE);

	/*		Connect Signales		*/
	bookSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.addressbookList));
	gtk_tree_selection_set_mode (bookSel, GTK_SELECTION_SINGLE);
	g_signal_connect(bookSel, "changed", G_CALLBACK(selBook), NULL);

	contactSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList));
	gtk_tree_selection_set_mode (contactSel, GTK_SELECTION_SINGLE);
	g_signal_connect(contactSel, "changed", G_CALLBACK(selContact), NULL);
	g_signal_connect(appBase.contactList, "button_press_event", G_CALLBACK(contactsTreeContextMenu), NULL);
	g_signal_connect(appBase.addressbookList, "button_press_event", G_CALLBACK(addressbookTreeContextMenu), NULL);

	g_signal_connect(G_OBJECT(ascContact), "clicked", G_CALLBACK(listSortorderAsc), NULL);
	g_signal_connect(G_OBJECT(descContact), "clicked", G_CALLBACK(listSortorderDesc), NULL);
	g_signal_connect(G_OBJECT(completion), "match-selected", G_CALLBACK(completionContact), NULL);

	g_signal_connect(G_OBJECT(delContact), "clicked", G_CALLBACK(contactDel), NULL);
	g_signal_connect(G_OBJECT(addContact), "clicked", G_CALLBACK(contactNew), NULL);

	g_signal_connect(G_OBJECT(editContact), "clicked", G_CALLBACK(contactEdit), NULL);

	g_signal_connect(G_OBJECT(appBase.window), "key_press_event", G_CALLBACK(guiKeyHandler), cleanUpList);
	g_signal_connect(G_OBJECT(appBase.window), "destroy", G_CALLBACK(guiExit), cleanUpList);
	g_signal_connect(G_OBJECT(prefItem), "clicked", G_CALLBACK(prefWindow), NULL);
	g_signal_connect(G_OBJECT(aboutItem), "clicked", G_CALLBACK(dialogAbout), NULL);
	g_signal_connect(G_OBJECT(newServer), "clicked", G_CALLBACK(newDialog), NULL);
	g_signal_connect(G_OBJECT(syncItem), "clicked", G_CALLBACK(syncServer), NULL);

	g_signal_connect(appBase.cal, "month-changed", G_CALLBACK (selBook), NULL);

	/*	Build the base structure 	*/
	appBase.statusbar 		= mainStatusbar;
	appBase.syncMenu 		= syncMenu;
	appBase.contactView		= scroll;
	gtk_widget_set_hexpand(GTK_WIDGET(appBase.contactView), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(appBase.contactView), TRUE);

	/*	capture some signals	*/
	g_unix_signal_add(SIGINT, exitOnSignal, cleanUpList);
	g_unix_signal_add(SIGTERM, exitOnSignal, cleanUpList);
	g_unix_signal_add(SIGHUP, exitOnSignal, cleanUpList);

	/*		Put it all together		*/
	gtk_box_pack_start(GTK_BOX(mainVBox), mainToolbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(addressbookWindow), appBase.addressbookList);
	gtk_box_pack_start(GTK_BOX(leftBox), GTK_WIDGET(addressbookWindow), TRUE, TRUE, 2);
    if((appBase.flags & CONTACTCARDS_HIDE_CAL) != CONTACTCARDS_HIDE_CAL){
        gtk_box_pack_end(GTK_BOX(leftBox), GTK_WIDGET(appBase.cal), FALSE, FALSE, 2);
    }
    gtk_container_add(GTK_CONTAINER(mainContent), leftBox);
	gtk_container_add(GTK_CONTAINER(contactWindow), appBase.contactList);
	gtk_container_add(GTK_CONTAINER(contactBox), contactsEdit);
	gtk_container_add(GTK_CONTAINER(contactBox), scroll);
	gtk_container_add(GTK_CONTAINER(mainContent), contactBox);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainContent, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainStatusbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(appBase.window), mainVBox);
	gtk_widget_show_all(appBase.window);
}
