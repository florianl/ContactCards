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
 * guiRun - run the basic GUI
 */
void guiRun(sqlite3 *ptr){
	printfunc(__func__);

	addressbookTreeUpdate();
	contactsTreeUpdate(0,0);
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
		"website", "https://www.der-flo.net/contactcards/",
		"license", "GNU General Public License, version 2\nhttp://www.gnu.org/licenses/old-licenses/gpl-2.0.html",
		"version", VERSION,
		NULL);
}

/**
 * selBook - select a single address book
 */
static void selBook(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeModel			*model;
	GtkTreeIter				iter;
	int						selID;
	int						selTyp;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.addressbookList)), &model, &iter)){
		gtk_tree_model_get (model, &iter, ID_COL, &selID, TYP_COL, &selTyp, -1);
		verboseCC("[%s] Typ: %d\tID:%d\n", __func__, selTyp, selID);
		switch(selTyp){
			case 0:		/* Whole Server selected	*/
				if(selID == 0){
					contactsTreeUpdate(0, 0);
				} else {
					verboseCC("[%s] whole server\n", __func__);
					contactsTreeUpdate(0, selID);
				}
				break;
			case 1:		/* Just one address book selected	*/
				contactsTreeUpdate(1, selID);
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
	gint 				resp;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		int				isOAuth = 0;
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);

		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Do you really want to delete this contact?"));

		resp = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if (resp != GTK_RESPONSE_YES) return;

		g_mutex_lock(&mutex);
		addrID = getSingleInt(appBase.db, "contacts", "addressbookID", 1, "contactID", selID, "", "", "", "");
		srvID = getSingleInt(appBase.db, "addressbooks", "cardServer", 1, "addressbookID", addrID, "", "", "", "");

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

		store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(appBase.contactList)));
		gtk_list_store_remove(store, &iter);

failure:
		g_mutex_unlock(&mutex);
	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no contact selected."));
	}
}

/**
 * contactDelcb - Callback for contactDel()
 */
static void contactDelcb(GtkMenuItem *menuitem, gpointer     user_data){
	printfunc(__func__);

	contactDel(NULL, NULL);
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
	g_strfreev(postalPtr);

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
			verboseCC("[%s] %s\n", __func__, error->message);
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

	ContactCards_add_t			*value = trans;
	char						*vCard = NULL;
	char						*dbCard = NULL;
	int							oldID = value->editID;
	int							addrID = value->aID;

	g_mutex_lock(&mutex);

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
	return NULL;
}

/**
 * checkInput - very simple input check
 */
gboolean checkInput(GSList *list){
	printfunc(__func__);

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
	printfunc(__func__);

	GError						*error = NULL;
	GThread						*thread;
	ContactCards_add_t			*value = trans;

	if(checkInput(value->list)== FALSE){
		feedbackDialog(GTK_MESSAGE_ERROR, _("Unable to save changes"));
		return;
	}
	cleanCard(((ContactCards_add_t *)trans)->grid);
	thread = g_thread_try_new("pushing vCard", pushingCard, trans, &error);
	if(error){
		verboseCC("[%s] something has gone wrong with threads\n", __func__);
		verboseCC("%s\n", error->message);
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
		g_strfreev(namingPtr);
	}

	transNew->db = ptr;
	transNew->grid = card;
	transNew->list = items;
	transNew->editID = selID;
	if(!abID)
		abID = getSingleInt(ptr, "contacts", "addressbookID", 1, "contactID", selID, "", "", "", "");
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
	int					abID = 0,
						selTyp = 0;
	GtkTreeIter			iter;
	GtkTreeModel		*model;


	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.addressbookList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, ID_COL, &abID, TYP_COL, &selTyp, -1);
	}

	verboseCC("[%s] %d\n",__func__, abID);

	if(abID == 0 || selTyp == 0){
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no address book selected."));
		return;
	}

	newCard = buildEditCard(appBase.db, 0, abID);
	cleanCard(appBase.contactView);
	gtk_widget_show_all(newCard);
	gtk_container_add(GTK_CONTAINER(appBase.contactView), newCard);
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

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);
	} else {
		feedbackDialog(GTK_MESSAGE_WARNING, _("There is no vCard selected to edit."));
		return;
	}

	editCard = buildEditCard(appBase.db, selID, 0);
	cleanCard(appBase.contactView);
	gtk_widget_set_halign (editCard, GTK_ALIGN_FILL);
	gtk_widget_set_valign (editCard, GTK_ALIGN_FILL);
	gtk_widget_show_all(editCard);
	gtk_container_add(GTK_CONTAINER(appBase.contactView), editCard);

}

/**
 * contactEditdb - Callback for contactEdit()
 */
static void contactEditcb(GtkMenuItem *menuitem, gpointer data){
	printfunc(__func__);

	contactEdit(NULL, NULL);
}


/**
 * completionContact - select a vCard from the searchbar
 */
static void completionContact(GtkEntryCompletion *widget, GtkTreeModel *model, GtkTreeIter *iter, gpointer trans){
	printfunc(__func__);

	GtkWidget					*card;
	int							selID;

	gtk_tree_model_get(model, iter, ID_COLUMN, &selID,  -1);
	verboseCC("[%s] %d\n",__func__, selID);
	card = buildNewCard(appBase.db, selID);
	gtk_widget_show_all(card);
	cleanCard(appBase.contactView);
	gtk_container_add(GTK_CONTAINER(appBase.contactView), card);
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

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);
		card = buildNewCard(appBase.db, selID);
		gtk_widget_show_all(card);
		cleanCard(appBase.contactView);
		gtk_container_add(GTK_CONTAINER(appBase.contactView), card);
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
	printfunc(__func__);

	GtkTreeStore		*store;

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.contactList)));

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LAST_COLUMN, GTK_SORT_ASCENDING);

}

/**
 * listSortorderDesc - sort a list descending
 */
static void listSortorderDesc(void){
	printfunc(__func__);

	GtkTreeStore		*store;

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.contactList)));

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LAST_COLUMN, GTK_SORT_DESCENDING);

}

/**
 * syncOneServer - check one single server for new data
 */
void *syncOneServer(void *trans){
	printfunc(__func__);

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
			free(srv);
			free(msg);
			return NULL;
		}
	}

	sess = serverConnect(serverID);
	syncContacts(appBase.db, sess, serverID);
	serverDisconnect(sess, appBase.db, serverID);

	gtk_statusbar_pop(GTK_STATUSBAR(appBase.statusbar), ctxID);

	free(srv);
	free(msg);
	g_mutex_unlock(&mutex);

	addressbookTreeUpdate();

	return NULL;
}

/**
 * addressbookDel - delete an address book from the server
 */
static void addressbookDel(GtkMenuItem *menuitem, gpointer data){
	printfunc(__func__);

	GtkWidget			*dialog;
	int					srvID, aID;
	ne_session 			*sess = NULL;
	int 				resp;
	int					isOAuth = 0;

	aID = GPOINTER_TO_INT(data);

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Do you really want to delete this address book?"));

	debugCC("[%s] deleting address book %d\n", __func__, aID);

	resp = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if (resp != GTK_RESPONSE_YES) return;

	g_mutex_lock(&mutex);
	srvID = getSingleInt(appBase.db, "addressbooks", "cardServer", 1, "addressbookID", aID, "", "", "", "");

	isOAuth = getSingleInt(appBase.db, "cardServer", "isOAuth", 1, "serverID", srvID, "", "", "", "");

	if(isOAuth){
		int 		ret = 0;
		ret = oAuthUpdate(appBase.db, srvID);
		if(ret != OAUTH_UP2DATE)
			goto failure;
	}

	sess = serverConnect(srvID);
	serverDelCollection(appBase.db, sess, srvID, aID);
	serverDisconnect(sess, appBase.db, srvID);
	addressbookTreeUpdate();

failure:
		g_mutex_unlock(&mutex);
}

/**
 * addressbookTreeContextMenu - a simple context menu for the address books view
 */
void addressbookTreeContextMenu(GtkWidget *widget, GdkEvent *event, gpointer data){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					selID;
	int					typ;
	int 				flags = 0;

	/*	right mouse button	*/
	if(event->button.button != 3)
		return;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.addressbookList))), &model, &iter)) {
		GtkWidget			*menu = NULL,
							*menuItem = NULL;
		gtk_tree_model_get(model, &iter, TYP_COL, &typ, ID_COL, &selID,  -1);
		verboseCC("[%s] typ: %d\tselID: %d\n",__func__, typ, selID);

		if(typ == 0 && selID == 0){
			verboseCC("[%s] generic item selected\n", __func__);
			return;
		}

		menu = gtk_menu_new();

		switch(typ){
			case 0:		/*	server	*/
				return;
				/*	Creating new address books isn't supported so far	*/
				flags = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", selID, "", "", "", "");
				if(flags & DAV_OPT_MKCOL){
					verboseCC("[%s] %d supports MKCOL\n", __func__, selID);
				} else {
					break;
				}
				verboseCC("[%s] Server %d selected\n", __func__, selID);
				menuItem = gtk_menu_item_new_with_label(_("Create new address book"));
				break;
			case 1:		/* address book	*/
				verboseCC("[%s] Adress book %d selected\n", __func__, selID);
				menuItem = gtk_menu_item_new_with_label(_("Delete address book"));
				g_signal_connect(menuItem, "activate", (GCallback)addressbookDel, GINT_TO_POINTER(selID));
				break;
		}
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button.button, gdk_event_get_time((GdkEvent*)event));
	}
}

/**
 * addressbookTreeUpdate - updates the address books view
 */
void addressbookTreeUpdate(void){
	printfunc(__func__);

	GtkTreeStore	*store;
	GSList			*servers, *addressBooks;
	GtkTreeIter		toplevel, child;

	g_mutex_lock(&aBookTreeMutex);

	/* Flush the tree	*/
	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.addressbookList)));
	gtk_tree_store_clear(store);

	/* Insert new elements	*/
	servers = getListInt(appBase.db, "cardServer", "serverID", 0, "", 0, "", "", "", "");
	gtk_tree_store_append(store, &toplevel, NULL);
	gtk_tree_store_set(store, &toplevel, DESC_COL, _("All"), ID_COL, 0, TYP_COL, 0,  -1);
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
}

/**
 * addressbookModelCreate - create the model for the address books view
 */
static GtkTreeModel *addressbookModelCreate(void){
	printfunc(__func__);

	GtkTreeStore  *treestore;

	treestore = gtk_tree_store_new(TOTAL_COLS, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);

	return GTK_TREE_MODEL(treestore);
}

/**
 * addressbookTreeCreate - creates the model and view for the adress books
 */
static GtkWidget *addressbookTreeCreate(void){
	printfunc(__func__);

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

	return view;
}

/**
 * contactsTreeContextMenu - a simple context menu for the list view
 */
void contactsTreeContextMenu(GtkWidget *widget, GdkEvent *event, gpointer data){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					selID;

	/*	right mouse button	*/
	if(event->button.button != 3)
		return;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(appBase.contactList))), &model, &iter)) {
		GtkWidget		*menu,
						*delItem,
						*editItem;

		gtk_tree_model_get(model, &iter, SELECTION_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);

		menu = gtk_menu_new();
		delItem = gtk_menu_item_new_with_label(_("Delete"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), delItem);
		g_signal_connect(delItem, "activate", (GCallback)contactDelcb, NULL);
		editItem = gtk_menu_item_new_with_label(_("Edit"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), editItem);
		g_signal_connect(editItem, "activate", (GCallback)contactEditcb, NULL);
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button.button, gdk_event_get_time((GdkEvent*)event));
	}
}

/**
 * contactsTreeAppend - append a new item to a contacts list view
 */
void contactsTreeAppend(char *card, int id){
	printfunc(__func__);

	GtkTreeStore		*store;
	GtkTreeIter 		iter;
	char				*fn = NULL,
						*n = NULL,
						*first  = NULL,
						*last = NULL;
	char				**nPtr = NULL;

	fn = getSingleCardAttribut(CARDTYPE_FN, card);
	if(strlen(g_strstrip(fn)) == 0)
		fn = g_strndup("(no name)", sizeof("(no name)"));

	n = getSingleCardAttribut(CARDTYPE_N, card);

	nPtr = g_strsplit(n, ";", 5);
	last = g_strndup(g_strstrip(nPtr[0]), sizeof(g_strstrip(nPtr[0])));
	first = g_strndup(g_strstrip(nPtr[1]), sizeof(g_strstrip(nPtr[1])));
	g_strfreev(nPtr);

	debugCC("[%s] first: %s\tlast: %s\n", __func__, first, last);

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.contactList)));

	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, FN_COLUMN, fn, FIRST_COLUMN, first, LAST_COLUMN, last, SELECTION_COLUMN, id, -1);

	g_free(fn);
	g_free(n);
	g_free(first);
	g_free(last);
}

/**
 * contactsTreeUpdate - updates the contacts list view
 */
void contactsTreeUpdate(int type, int id){
	printfunc(__func__);

	GtkTreeStore	*store;
	GSList			*contacts = NULL;

	g_mutex_lock(&contactsTreeMutex);

	/* Flush the tree	*/
	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (appBase.contactList)));
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
					contactsTreeFill(contacts);
					g_slist_free(contacts);
					addressBooks = next;
				}
				g_slist_free(addressBooks);
				g_mutex_unlock(&contactsTreeMutex);
				return;
			}
			break;
		case 1:		/*	address book selected	*/
			contacts = getListInt(appBase.db, "contacts", "contactID", 1, "addressbookID", id, "", "", "", "");
			break;
		default:
			break;
	}
	contactsTreeFill(contacts);
	g_slist_free(contacts);
	g_mutex_unlock(&contactsTreeMutex);
}

/**
 * contactsModelCreate - create the model for the contacts list view
 */
static GtkTreeModel *contactsModelCreate(void){
	printfunc(__func__);

	GtkTreeStore  *treestore;

	treestore = gtk_tree_store_new(TOTAL_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT);

	return GTK_TREE_MODEL(treestore);
}

/**
 * contactsTreeCreate - creates the model and view for the contacts list
 */
static GtkWidget *contactsTreeCreate(void){
	printfunc(__func__);

	GtkWidget				*view;
	GtkTreeViewColumn		*column;
	GtkTreeModel			*model;
	GtkCellRenderer			*renderer;

	view = gtk_tree_view_new();

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "text", DESC_COL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);

	model = contactsModelCreate();
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_SINGLE);

	return view;
}

/**
 * dialogExportContacts - dialog to export vCards
 */
static void dialogExportContacts(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget					*dirChooser;
	int							result;
	char						*path = NULL;

	dirChooser = gtk_file_chooser_dialog_new(_("Export Contacts"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);

	g_signal_connect(G_OBJECT(dirChooser), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	result = gtk_dialog_run(GTK_DIALOG(dirChooser));

	switch(result){
		case GTK_RESPONSE_ACCEPT:
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dirChooser));
			exportContacts(appBase.db, path);
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

	GSList						*retList;
	GError		 				*error = NULL;
	GThread						*thread;

	retList = getListInt(appBase.db, "cardServer", "serverID", 0, "", 0, "", "", "", "");

	if(g_slist_length(retList) == 1){
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
		g_mutex_lock(&mutex);
		thread = g_thread_try_new("syncingServer", syncOneServer, GINT_TO_POINTER(serverID), &error);
		if(error){
			verboseCC("[%s] something has gone wrong with threads\n", __func__);
			verboseCC("%s\n", error->message);
			g_mutex_unlock(&mutex);
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
	printfunc(__func__);

	GtkWidget			*mainVBox, *mainToolbar, *mainStatusbar, *mainContent;
	GtkWidget			*addressbookWindow;
	GtkWidget			*contactBox, *contactWindow, *scroll;
	GtkWidget			*addContact, *delContact, *contactButtons, *contactsEdit, *editContact;
	GtkWidget			*ascContact, *descContact, *searchbar;
	GtkWidget			*emptyCard, *noContact;
	GtkWidget			*syncMenu;
	GtkToolItem			*prefItem, *aboutItem, *sep, *newServer, *syncItem, *exportItem;
	GtkTreeSelection	*bookSel, *contactSel;
	GSList 				*cleanUpList = g_slist_alloc();
	GtkEntryCompletion	*completion;

	gtk_init(NULL, NULL);

	appBase.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(appBase.window), "ContactCards");
	gtk_window_set_default_size(GTK_WINDOW(appBase.window), 760,496);

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
	appBase.statusbar = mainStatusbar;

	/*	Sync Menu					*/
	syncMenuUpdate();

	/*		mainContent				*/
	mainContent = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	/*		Addressbookstuff		*/
	addressbookWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(addressbookWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(addressbookWindow, 160, -1);
	appBase.addressbookList = addressbookTreeCreate();

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
	searchbar = gtk_entry_new();
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(searchbar), GTK_ENTRY_ICON_SECONDARY, "stock_search");
	gtk_entry_set_completion(GTK_ENTRY(searchbar), GTK_ENTRY_COMPLETION(completion));
	gtk_widget_set_vexpand(GTK_WIDGET(appBase.contactList), TRUE);
	gtk_container_add(GTK_CONTAINER(contactsEdit), contactWindow);
	gtk_container_add(GTK_CONTAINER(contactsEdit), searchbar);
	gtk_container_add(GTK_CONTAINER(contactsEdit), contactButtons);

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
	g_signal_connect(G_OBJECT(exportItem), "clicked", G_CALLBACK(dialogExportContacts), NULL);

	/*	Build the base structure 	*/
	appBase.statusbar 		= mainStatusbar;
	appBase.syncMenu 		= syncMenu;
	appBase.contactView		= scroll;

	/*		Put it all together		*/
	gtk_box_pack_start(GTK_BOX(mainVBox), mainToolbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(addressbookWindow), appBase.addressbookList);
	gtk_container_add(GTK_CONTAINER(mainContent), addressbookWindow);
	gtk_container_add(GTK_CONTAINER(contactWindow), appBase.contactList);
	gtk_container_add(GTK_CONTAINER(contactBox), contactsEdit);
	gtk_container_add(GTK_CONTAINER(contactBox), scroll);
	gtk_container_add(GTK_CONTAINER(mainContent), contactBox);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainContent, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainVBox), mainStatusbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(appBase.window), mainVBox);
	gtk_widget_show_all(appBase.window);
}
