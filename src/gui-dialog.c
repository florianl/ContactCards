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
			if(validateUrl((char *) gtk_entry_buffer_get_text(buf2)) == FALSE){
				gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), box, FALSE);
				return;
			}
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
	newServer(appBase.db, (char *) gtk_entry_buffer_get_text(buf3), (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), (char *) gtk_entry_buffer_get_text(buf4));
	syncMenuUpdate();
	addressbookTreeUpdate();
	return;

fruux:
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 2);
	buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "userEntry");
	buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "passwdEntry");
	newServer(appBase.db, "fruux", (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), "https://dav.fruux.com");
	syncMenuUpdate();
	addressbookTreeUpdate();
	return;

google:
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 3);
	buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "oAuthEntry");
	buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "grantEntry");
	newServerOAuth(appBase.db, "google", (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), 1);
	syncMenuUpdate();
	addressbookTreeUpdate();
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

	g_free(uri);
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
void newDialog(GtkWidget *do_widget, gpointer trans){
	printfunc(__func__);

	GtkWidget 			*assistant = NULL;
	GError				*error = NULL;
	GdkPixbuf			*pixbuf;

	assistant = gtk_assistant_new ();
	gtk_window_set_destroy_with_parent(GTK_WINDOW(assistant), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (assistant), -1, 300);

	pixbuf = gdk_pixbuf_new_from_file("artwork/icon_48.png", &error);
	if(error){
		verboseCC("[%s] something has gone wrong\n", __func__);
		verboseCC("%s\n", error->message);
	}
	gtk_window_set_icon(GTK_WINDOW(assistant), pixbuf);
	g_object_unref(pixbuf);

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
	g_signal_connect(G_OBJECT(assistant), "apply", G_CALLBACK(newDialogApply), NULL);
}

/**
 * syncMenuSel - Callback for sync menu
 */
static void syncMenuSel(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GError		 				*error = NULL;
	GThread						*thread;

	verboseCC("[%s] you selected %d\n", __func__, GPOINTER_TO_INT(trans));

	thread = g_thread_try_new("syncingServer", syncOneServer, trans, &error);
	if(error){
		verboseCC("[%s] something has gone wrong with threads\n", __func__);
		verboseCC("%s\n", error->message);
	}
	g_thread_unref(thread);

}

/**
 * syncMenuItem - one item of the sync menu
 */
static GtkWidget *syncMenuItem(int sID){
	printfunc(__func__);

	GtkWidget				*item;
	char					*desc = NULL;

	desc = getSingleChar(appBase.db, "cardServer", "desc", 1, "serverID", sID, "", "", "", "", "", 0);
	item = gtk_menu_item_new_with_label(desc);
	g_free(desc);

	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(syncMenuSel), GINT_TO_POINTER(sID));
	gtk_widget_show_all(item);

	return item;
}

/**
 * buildRow - creates a row for the list of address books
 */
static GtkWidget *buildRow(sqlite3 *ptr, int aID, GSList *list){
	printfunc(__func__);

	GtkWidget		*row, *box, *check, *label;
	char			*abName = NULL;
	int				active = 0;
	ContactCards_aBooks_t		*element;

	element = g_new(ContactCards_aBooks_t, 1);

	row = gtk_list_box_row_new ();
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	check = gtk_check_button_new();
	active = getSingleInt(ptr, "addressbooks", "syncMethod", 1, "addressbookID", aID, "", "", "", "");

	if(active & (1<<DAV_ADDRBOOK_DONT_SYNC)){
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
	}
	abName = getSingleChar(ptr, "addressbooks", "displayname", 1, "addressbookID", aID, "", "", "", "", "", 0);
	label = gtk_label_new(abName);
	g_free(abName);

	element->aBookID = aID;
	element->check = check;
	list = g_slist_append(list, element);

	gtk_container_add(GTK_CONTAINER (box), check);
	gtk_container_add(GTK_CONTAINER (box), label);
	gtk_container_add(GTK_CONTAINER (row), box);
	gtk_widget_show_all(row);

	return row;
}

/**
 * prefServerDelete - delete a server in the preferences dialog
 */
void prefServerDelete(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_pref_t		*buffers = trans;
	GList					*children, *iter;

	verboseCC("[%s] %d\n", __func__, buffers->srvID);

	if(buffers->srvID == 0){
		verboseCC("[%s] this isn't a server\n", __func__);
		return;
	}

	dbRemoveItem(appBase.db, "cardServer", 2, "", "", "serverID", buffers->srvID);
	dbRemoveItem(appBase.db, "certs", 2, "", "", "serverID", buffers->srvID);
	cleanUpRequest(appBase.db, buffers->srvID, 0);
	fillList(appBase.db, 3, 0, 0, buffers->srvPrefList);
	addressbookTreeUpdate();
	syncMenuUpdate();

	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->descBuf), "", -1);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->urlBuf), "", -1);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->userBuf), "", -1);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), "", -1);

	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuedBuf), "", -1);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuerBuf), "", -1);
	gtk_switch_set_active(GTK_SWITCH(buffers->certSel), FALSE);

	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->descBuf), "", -1);

	children = gtk_container_get_children(GTK_CONTAINER(buffers->listbox));
	for(iter = children; iter != NULL; iter = g_list_next(iter)) {
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);
}

/**
 * prefServerSave - save changes to a server in the preferences dialog
 */
void prefServerSave(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_pref_t		*buffers = trans;

	if(buffers->srvID == 0){
		verboseCC("[%s] this isn't a server\n", __func__);
		return;
	}

	if(validateUrl((char *)gtk_entry_buffer_get_text(buffers->urlBuf)) == FALSE){
		feedbackDialog(GTK_MESSAGE_ERROR, _("Seems like that is not a valid URL"));
		return;
	}

	updateAddressbooks(appBase.db, buffers->aBooks);
	updateServerDetails(appBase.db, buffers->srvID,
						gtk_entry_buffer_get_text(buffers->descBuf), gtk_entry_buffer_get_text(buffers->urlBuf), gtk_entry_buffer_get_text(buffers->userBuf), gtk_entry_buffer_get_text(buffers->passwdBuf),
						gtk_switch_get_active(GTK_SWITCH(buffers->certSel)),
						gtk_switch_get_active(GTK_SWITCH(buffers->syncSel)));
	addressbookTreeUpdate();
}

/**
 * prefServerCheck - checking for address books
 */
void prefServerCheck(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_pref_t			*buffers = trans;
	int							isOAuth = 0;
	ne_session					*sess = NULL;
	GList						*children, *iter;
	GSList						*abList;

	if(buffers->srvID == 0){
		verboseCC("[%s] this isn't a server\n", __func__);
		return;
	}

	while(g_mutex_trylock(&mutex) != TRUE){}

	isOAuth = getSingleInt(appBase.db, "cardServer", "isOAuth", 1, "serverID", buffers->srvID, "", "", "", "");

	if(isOAuth){
		int		ret = 0;
		ret = oAuthUpdate(appBase.db, buffers->srvID);
		if(ret != OAUTH_UP2DATE){
			g_mutex_unlock(&mutex);
			return;
		}
	}

	sess = serverConnect(buffers->srvID);
	syncInitial(appBase.db, sess, buffers->srvID);
	serverDisconnect(sess, appBase.db, buffers->srvID);

	g_mutex_unlock(&mutex);

	/* Flush the list box before adding new items	*/
	children = gtk_container_get_children(GTK_CONTAINER(buffers->listbox));
	for(iter = children; iter != NULL; iter = g_list_next(iter)) {
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	if (g_slist_length(buffers->aBooks) > 1){
		/* Yes, you're right. It's still really ugly	*/
		g_slist_free_full(buffers->aBooks, g_free);
		buffers->aBooks = g_slist_alloc();
	}

	abList = getListInt(appBase.db, "addressbooks", "addressbookID", 1, "cardServer", buffers->srvID, "", "", "", "");

	while(abList){
		GSList				*next = abList->next;
		GtkWidget			*row;

		if(GPOINTER_TO_INT(abList->data) == 0){
			abList = next;
			continue;
		}

		row = buildRow(appBase.db, GPOINTER_TO_INT(abList->data), buffers->aBooks);
		gtk_list_box_insert(GTK_LIST_BOX(buffers->listbox), row, -1);
		abList = next;
	}
	g_slist_free(abList);


	return;
}

/**
 * prefExportCert - export the certificate of a server
 */
void prefExportCert(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	ContactCards_pref_t		*buffers = trans;
	GtkWidget					*dirChooser;
	int							result;
	char						*path = NULL;

	if(buffers->srvID == 0){
		verboseCC("[%s] this isn't a server\n", __func__);
		return;
	}

	dirChooser = gtk_file_chooser_dialog_new(_("Export Certificate"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Export"), GTK_RESPONSE_ACCEPT, NULL);

	g_signal_connect(G_OBJECT(dirChooser), "key_press_event", G_CALLBACK(dialogKeyHandler), NULL);

	result = gtk_dialog_run(GTK_DIALOG(dirChooser));

	if (result == GTK_RESPONSE_ACCEPT) {
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dirChooser));
			exportCert(appBase.db, path, buffers->srvID);
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
	GSList				*abList;
	ContactCards_pref_t		*buffers = trans;
	char				*frameTitle = NULL, *user = NULL, *passwd = NULL;
	char				*issued = NULL, *issuer = NULL, *url = NULL;
	int					isOAuth;
	gboolean			res = 0;

	prefFrame = buffers->prefFrame;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		GList *children, *iter2;

		gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
		verboseCC("[%s] %d\n",__func__, selID);
		frameTitle = getSingleChar(appBase.db, "cardServer", "desc", 1, "serverID", selID, "", "", "", "", "", 0);
		if (frameTitle == NULL) return;
		gtk_frame_set_label(GTK_FRAME(prefFrame), frameTitle);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->descBuf), frameTitle, -1);

		buffers->srvID = selID;

		user = getSingleChar(appBase.db, "cardServer", "user", 1, "serverID", selID, "", "", "", "", "", 0);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->userBuf), user, -1);

		isOAuth = getSingleInt(appBase.db, "cardServer", "isOAuth", 1, "serverID", selID, "", "", "", "");
		if(!isOAuth){
			passwd = getSingleChar(appBase.db, "cardServer", "passwd", 1, "serverID", selID, "", "", "", "", "", 0);
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), passwd, -1);
		} else {
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), "", -1);
		}

		url = getSingleChar(appBase.db, "cardServer", "srvUrl", 1, "serverID", selID, "", "", "", "", "", 0);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->urlBuf), url, -1);

		gtk_switch_set_active(GTK_SWITCH(buffers->certSel), FALSE);

		if(countElements(appBase.db, "certs", 1, "serverID", selID, "", "", "", "") == 1){

			issued = getSingleChar(appBase.db, "certs", "issued", 1, "serverID", selID, "", "", "", "", "", 0);
			if(issued == NULL) issued = "";
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuedBuf), issued, -1);

			issuer = getSingleChar(appBase.db, "certs", "issuer", 1, "serverID", selID, "", "", "", "", "", 0);
			if(issuer == NULL) issuer = "";
			gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuerBuf), issuer, -1);

			res = getSingleInt(appBase.db, "certs", "trustFlag", 1, "serverID", selID, "", "", "", "");
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

		res = 0;
		res = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", selID, "", "", "", "");
		if(res & CONTACTCARDS_ONE_WAY_SYNC){
			gtk_switch_set_active(GTK_SWITCH(buffers->syncSel), TRUE);
		} else {
			gtk_switch_set_active(GTK_SWITCH(buffers->syncSel), FALSE);
		}

		abList = getListInt(appBase.db, "addressbooks", "addressbookID", 1, "cardServer", selID, "", "", "", "");

		/* Flush the list box before adding new items	*/
		children = gtk_container_get_children(GTK_CONTAINER(buffers->listbox));
		for(iter2 = children; iter2 != NULL; iter2 = g_list_next(iter2)) {
			gtk_widget_destroy(GTK_WIDGET(iter2->data));
		}
		g_list_free(children);

		if (g_slist_length(buffers->aBooks) > 1){
			/* Yes, you're right. It's really ugly	*/
			g_slist_free_full(buffers->aBooks, g_free);
			buffers->aBooks = g_slist_alloc();
		}

		while(abList){
			GSList				*next = abList->next;
			GtkWidget			*row;

			if(GPOINTER_TO_INT(abList->data) == 0){
				abList = next;
				continue;
			}

			row = buildRow(appBase.db, GPOINTER_TO_INT(abList->data), buffers->aBooks);
			gtk_list_box_insert(GTK_LIST_BOX(buffers->listbox), row, -1);
			abList = next;
		}
	}

	g_slist_free(abList);
	g_free(frameTitle);
	g_free(user);
	g_free(passwd);
	g_free(issued);
	g_free(issuer);
	g_free(url);
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
	GtkWidget			*vbox, *hbox, *abbox;
	GtkWidget			*label, *input;
	GtkWidget			*saveBtn, *deleteBtn, *exportCertBtn, *checkBtn;
	GtkWidget			*digSwitch, *uploadSwitch;
	GtkWidget			*sep;
	GtkWidget			*ablist;
	GtkEntryBuffer		*desc, *url, *user, *passwd;
	GtkEntryBuffer		*issued, *issuer;
	GtkTreeSelection	*serverSel;
	GSList				*aBooks = g_slist_alloc();
	GError				*error = NULL;
	GdkPixbuf			*pixbuf;

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

	pixbuf = gdk_pixbuf_new_from_file("artwork/icon_48.png", &error);
	if(error){
		verboseCC("[%s] something has gone wrong\n", __func__);
		verboseCC("%s\n", error->message);
	}
	gtk_window_set_icon(GTK_WINDOW(prefWindow), pixbuf);
	g_object_unref(pixbuf);

	prefView = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	prefList = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(prefList), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(prefList, 128, -1);

	prefFrame = gtk_frame_new(_("Settings"));
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
	digSwitch = gtk_switch_new();
	gtk_box_pack_start(GTK_BOX(hbox), digSwitch, FALSE, TRUE, 2);
	label = gtk_label_new(_("Trust Certificate?"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	label = gtk_label_new(_("One-Way-Sync"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	uploadSwitch = gtk_switch_new();
	gtk_box_pack_start(GTK_BOX(hbox), uploadSwitch, FALSE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	checkBtn = gtk_button_new_with_label(_("Check for address books"));
	gtk_box_pack_start(GTK_BOX(hbox), checkBtn, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	abbox = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(abbox), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(abbox, 256, 128);
	ablist = gtk_list_box_new();
	gtk_container_add(GTK_CONTAINER(abbox), ablist);
	gtk_box_pack_start(GTK_BOX(hbox), abbox, FALSE, FALSE, 2);
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
	fillList(appBase.db, 3, 0, 0, serverPrefList);

	buffers->prefFrame = prefFrame;
	buffers->descBuf = desc;
	buffers->urlBuf = url;
	buffers->userBuf = user;
	buffers->passwdBuf = passwd;
	buffers->issuedBuf = issued;
	buffers->issuerBuf = issuer;
	buffers->srvPrefList = serverPrefList;
	buffers->certSel = digSwitch;
	buffers->syncSel = uploadSwitch;
	buffers->listbox = ablist;
	buffers->aBooks = aBooks;

	/*		Connect Signales		*/
	serverSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(serverPrefList));
	gtk_tree_selection_set_mode (serverSel, GTK_SELECTION_SINGLE);
	g_signal_connect(serverSel, "changed", G_CALLBACK(prefServerSelect), buffers);
	g_signal_connect(G_OBJECT(prefWindow), "destroy", G_CALLBACK(prefExit), buffers);

	g_signal_connect(deleteBtn, "clicked", G_CALLBACK(prefServerDelete), buffers);
	g_signal_connect(saveBtn, "clicked", G_CALLBACK(prefServerSave), buffers);
	g_signal_connect(exportCertBtn, "clicked", G_CALLBACK(prefExportCert), buffers);
	g_signal_connect(checkBtn, "clicked", G_CALLBACK(prefServerCheck), buffers);

	g_signal_connect(G_OBJECT(prefWindow), "key_press_event", G_CALLBACK(prefKeyHandler), buffers);

	gtk_container_add(GTK_CONTAINER(prefView), prefList);
	gtk_container_add(GTK_CONTAINER(prefView), prefFrame);
	gtk_container_add(GTK_CONTAINER(prefWindow), prefView);
	gtk_widget_show_all(prefWindow);
}

/**
 * syncMenuFill - fills the menu with items
 */
static void syncMenuFill(void){
	printfunc(__func__);

	GSList		*list;
	GtkWidget	*item;

	list = getListInt(appBase.db, "cardServer", "serverID", 0, "", 0, "", "", "", "");
	while(list){
		GSList				*next = list->next;
		int					sID = GPOINTER_TO_INT(list->data);
		if(sID == 0){
			list = next;
			continue;
		}
		item = syncMenuItem(sID);
		gtk_menu_shell_append(GTK_MENU_SHELL(appBase.syncMenu), item);
		list = next;
	}
	g_slist_free(list);
}

/**
 * syncMenuFlush - delete all items from menu
 */
static void syncMenuFlush(void){
	printfunc(__func__);

	GList			*children, *iter;

	children = gtk_container_get_children(GTK_CONTAINER(appBase.syncMenu));
	for(iter = children; iter != NULL; iter = g_list_next(iter)) {
			gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);
}

/**
 * syncMenuUpdate - updates the menu
 */
void syncMenuUpdate(void){
	printfunc(__func__);

	syncMenuFlush();
	syncMenuFill();
}

/**
 * markDay - mark the birthday of each contact in the calendar
 */
void markDay(GSList *contacts, GtkWidget *cal){
	printfunc(__func__);

	while(contacts){
		GSList				*next =  contacts->next;
		int					id = GPOINTER_TO_INT(contacts->data);
		int					flags = 0;
		char				*card = NULL;
		char				*bday = NULL;

		if(id == 0){
			contacts = next;
			continue;
		}

		flags = getSingleInt(appBase.db, "contacts", "flags", 1, "contactID", id, "", "", "", "");
		if(flags & CONTACTCARDS_TMP){
			debugCC("[%s] hiding %d\n", __func__, id);
			contacts = next;
			continue;
		}

		card = getSingleChar(appBase.db, "contacts", "vCard", 1, "contactID", id, "", "", "", "", "", 0);

		bday = getSingleCardAttribut(CARDTYPE_BDAY, card);
		if(bday != NULL){
			GDate		*date;
			int			month;
			date = g_date_new();
			g_date_set_parse(date, bday);
			if(g_date_valid(date) == TRUE){
				gtk_calendar_get_date(GTK_CALENDAR(cal), NULL, &month, NULL);
				if(g_date_get_month(date) == (month+1)){
					gtk_calendar_mark_day(GTK_CALENDAR(cal), (int)g_date_get_day(date));
				}
			}
			g_date_free(date);
			g_free(bday);
			bday = NULL;
		}
		g_free(card);
		contacts = next;
	}
}

/**
 * calendarUpdate - Update the birthday calendar
 */
void calendarUpdate(GtkWidget *cal, int type, int id){
	printfunc(__func__);

	GSList			*contacts = NULL;

	/*	Clean up at first	*/
	gtk_calendar_clear_marks (GTK_CALENDAR(cal));

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
					markDay(contacts, cal);
					g_slist_free(contacts);
					addressBooks = next;
				}
				g_slist_free(addressBooks);
				return;
			}
			break;
		case 1:		/*	address book selected	*/
			contacts = getListInt(appBase.db, "contacts", "contactID", 1, "addressbookID", id, "", "", "", "");
			break;
		default:
			break;
	}
	markDay(contacts, cal);
	g_slist_free(contacts);

}

/**
 * selABook - select a server or address book to show the birthdays
 */
static void selABook(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkTreeModel			*model;
	GtkTreeIter				iter;
	int						selID;
	int						selTyp;
	ContactCards_cal_t		*data = trans;

	if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->tree)), &model, &iter)){
		gtk_tree_model_get (model, &iter, ID_COL, &selID, TYP_COL, &selTyp, -1);
		verboseCC("[%s] Typ: %d\tID:%d\n", __func__, selTyp, selID);
		switch(selTyp){
			case 0:		/* Whole Server selected	*/
				if(selID == 0){
					calendarUpdate(data->cal, 0, 0);
				} else {
					verboseCC("[%s] whole server\n", __func__);
					calendarUpdate(data->cal, 0, selID);
				}
				break;
			case 1:		/* Just one address book selected	*/
				calendarUpdate(data->cal, 1, selID);
				break;
		}
	}

}

/**
 * addressbooksUpdate - it's a own function to not mix it up with the main
 */
void addressbooksUpdate(GtkWidget *widget){
	printfunc(__func__);

	GtkTreeStore	*store;
	GSList			*servers, *addressBooks;
	GtkTreeIter		toplevel, child;

	while(g_mutex_trylock(&aBookTreeMutex) != TRUE){}

	/* Flush the tree	*/
	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW (widget)));
	/*	something is wrong	*/
	if(store == NULL){
		g_mutex_unlock(&aBookTreeMutex);
		return;
	}

	gtk_tree_store_clear(store);

	/* Insert new elements	*/
	servers = getListInt(appBase.db, "cardServer", "serverID", 0, "", 0, "", "", "", "");
	gtk_tree_store_append(store, &toplevel, NULL);
	gtk_tree_store_set(store, &toplevel, DESC_COL, _("All"), ID_COL, 0, TYP_COL, 0,  -1);
	if(g_slist_length(servers) == 0){
		g_mutex_unlock(&aBookTreeMutex);
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
 * bdExit - exit the birthday dialog
 */
void bdExit(GtkWidget *widget, gpointer data){
	printfunc(__func__);

	g_free(data);
}

/**
 * birthdayDialogTreeContextMenu - a simple context menu
 */
void birthdayDialogTreeContextMenu(GtkWidget *widget, GdkEvent *event, gpointer data){
	printfunc(__func__);

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	int					selID;
	int					typ;

	/*	right mouse button	*/
	if(event->button.button != 3)
		return;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(data))), &model, &iter)) {
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
				verboseCC("[%s] Server %d selected\n", __func__, selID);
				menuItem = gtk_menu_item_new_with_label(_("Export Birthdays"));
				g_signal_connect(menuItem, "activate", (GCallback)cbSrvExportBirthdays, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
				break;
			case 1:		/* address book	*/
				verboseCC("[%s] Adress book %d selected\n", __func__, selID);
				menuItem = gtk_menu_item_new_with_label(_("Export Birthdays"));
				g_signal_connect(menuItem, "activate", (GCallback)cbAddrBookExportBirthdays, GINT_TO_POINTER(selID));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
				break;
		}
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button.button, gdk_event_get_time((GdkEvent*)event));
	}
}

/**
 * birthdayDialog - a simple calendar showing birthdays
 */
void birthdayDialog(GtkWidget *widget, gpointer trans){
	printfunc(__func__);

	GtkWidget			*bdWindow, *cal, *splitView, *treeView;
	GtkWidget			*addressbookList;
	GtkTreeSelection	*bookSel;
	GError				*error = NULL;
	GdkPixbuf			*pixbuf;
	ContactCards_cal_t	*transCal;

	transCal = g_new(ContactCards_cal_t, 1);

	bdWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(bdWindow), _("Birthday Calendar"));
	gtk_window_resize(GTK_WINDOW(bdWindow), 384, 208);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(bdWindow), TRUE);

	pixbuf = gdk_pixbuf_new_from_file("artwork/icon_48.png", &error);
	if(error){
		verboseCC("[%s] something has gone wrong\n", __func__);
		verboseCC("%s\n", error->message);
	}
	gtk_window_set_icon(GTK_WINDOW(bdWindow), pixbuf);
	g_object_unref(pixbuf);

	splitView = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	cal = gtk_calendar_new();

	treeView = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(treeView, 128, -1);

	addressbookList = addressbookTreeCreate();

	addressbooksUpdate(addressbookList);

	transCal->cal = cal;
	transCal->tree = addressbookList;

	/*		Connect Signales		*/
	bookSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(addressbookList));
	gtk_tree_selection_set_mode (bookSel, GTK_SELECTION_SINGLE);
	g_signal_connect(bookSel, "changed", G_CALLBACK(selABook), transCal);
	g_signal_connect(addressbookList, "button_press_event", G_CALLBACK(birthdayDialogTreeContextMenu), (void*) addressbookList);
	g_signal_connect (cal, "month-changed", G_CALLBACK (selABook), transCal);
	g_signal_connect(G_OBJECT(bdWindow), "destroy", G_CALLBACK(bdExit), transCal);

	gtk_container_add(GTK_CONTAINER(treeView), addressbookList);
	gtk_container_add(GTK_CONTAINER(splitView), treeView);
	gtk_container_add(GTK_CONTAINER(splitView), cal);
	gtk_container_add(GTK_CONTAINER(bdWindow), splitView);
	gtk_widget_show_all(bdWindow);
}
