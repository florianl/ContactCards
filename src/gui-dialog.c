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

#include "contactcards.h"

void requestPasswd(credits_t *key, int serverID){
	__PRINTFUNC__;

	GtkWidget			*dialog, *content, *view;
	GtkWidget			*label, *input;
	GtkEntryBuffer		*username, *passwd;
	char				*user = NULL;
	gint 				resp;
	int					line = 0;

	dialog = gtk_dialog_new_with_buttons (_("Request for Password"), NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, _("OK"), GTK_RESPONSE_ACCEPT, _("Cancel"), GTK_RESPONSE_REJECT, NULL);

	content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	view = gtk_grid_new();
	username = gtk_entry_buffer_new(NULL, -1);
	passwd = gtk_entry_buffer_new(NULL, -1);

	label = gtk_label_new(_("Username"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(view), label, 1, line, 1, 1);
	input = gtk_entry_new_with_buffer(username);
	gtk_widget_set_margin_top(GTK_WIDGET(input), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(input), 3);
	gtk_grid_attach(GTK_GRID(view), input, 2, line++, 3, 1);

	/*	If there is a username, use it	*/
	user = getSingleChar(appBase.db, "cardServer", "user", 1, "serverID", serverID, "", "", "", "", "", 0);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(username), user, -1);

	label = gtk_label_new(_("Password"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(view), label, 1, line, 1, 1);
	input = gtk_entry_new_with_buffer(passwd);
	gtk_entry_set_visibility(GTK_ENTRY(input), FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(input), GTK_INPUT_PURPOSE_PASSWORD);
	gtk_widget_set_margin_top(GTK_WIDGET(input), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(input), 3);
	gtk_grid_attach(GTK_GRID(view), input, 2, line++, 3, 1);

	gtk_container_add (GTK_CONTAINER (content), view);
	gtk_widget_show_all (dialog);

	resp = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if (resp != GTK_RESPONSE_ACCEPT) return;

	if(strlen(gtk_entry_buffer_get_text(username)) < 1 ) return;
	if(strlen(gtk_entry_buffer_get_text(passwd)) < 1 ) return;
	key->user = g_strdup(gtk_entry_buffer_get_text(username));
	key->passwd = g_strdup(gtk_entry_buffer_get_text(passwd));

	return;
}

/**
 * newDialogEntryChanged - check for changes of a server in the preferences dialog
 */
static void newDialogEntryChanged(GtkWidget *widget, gpointer data){
	__PRINTFUNC__;

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
	__PRINTFUNC__;

	gtk_widget_destroy(widget);
}

/**
 * newDialogApply - apply the user credentials to a new server
 */
static void newDialogApply(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GtkWidget					*assistant, *box;
	GtkWidget					*controller;
	GtkEntryBuffer				*buf1, *buf2, *buf3, *buf4;
	gboolean					savePasswd = FALSE;

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
	savePasswd = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON((g_object_get_data(G_OBJECT(box), "sPasswd"))));
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 1);
	buf3 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "descEntry");
	buf4 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "urlEntry");
	newServer(appBase.db, savePasswd, (char *) gtk_entry_buffer_get_text(buf3), (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), (char *) gtk_entry_buffer_get_text(buf4));
	syncMenuUpdate();
	addressbookTreeUpdate();
	return;

fruux:
	box = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), 2);
	buf1 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "userEntry");
	buf2 = (GtkEntryBuffer*) g_object_get_data(G_OBJECT(box), "passwdEntry");
	newServer(appBase.db, FALSE, "fruux", (char *) gtk_entry_buffer_get_text(buf1), (char *) gtk_entry_buffer_get_text(buf2), "https://dav.fruux.com");
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
	__PRINTFUNC__;

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
	__PRINTFUNC__;

	GtkWidget				*box;
	GtkWidget				*label, *inputoAuth, *inputGrant, *button;
	GtkEntryBuffer			*user, *grant;
	char					*uri = NULL;
	int						line = 0;

	user = gtk_entry_buffer_new(NULL, -1);
	grant = gtk_entry_buffer_new(NULL, -1);

	uri = g_strdup("https://accounts.google.com/o/oauth2/auth?scope=https://www.googleapis.com/auth/carddav&redirect_uri=urn:ietf:wg:oauth:2.0:oob&response_type=code&client_id=741969998490.apps.googleusercontent.com");

	box = gtk_grid_new();

	label = gtk_label_new(_("User"));
	gtk_widget_set_margin_start(label, 12);
	gtk_widget_set_margin_end(label, 12);
	gtk_widget_set_margin_top(label, 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(box), label, 0, line, 1, 1);
	inputoAuth = gtk_entry_new_with_buffer(user);
	gtk_widget_set_hexpand(inputoAuth, TRUE);
	g_object_set_data(G_OBJECT(box),"oAuthEntry", user);
	gtk_grid_attach(GTK_GRID(box), inputoAuth, 1, line++, 1, 1);


	button = gtk_link_button_new_with_label(uri, _("Request Grant"));
	gtk_grid_attach(GTK_GRID(box), button, 1, line++, 1, 1);

	label = gtk_label_new(_("Grant"));
	gtk_widget_set_margin_start(label, 12);
	gtk_widget_set_margin_end(label, 12);
	gtk_widget_set_margin_top(label, 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(box), label, 0, line, 1, 1);
	inputGrant = gtk_entry_new_with_buffer(grant);
	gtk_widget_set_hexpand(inputGrant, TRUE);
	g_object_set_data(G_OBJECT(box),"grantEntry", grant);
	gtk_grid_attach(GTK_GRID(box), inputGrant, 1, line++, 1, 1);

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
	__PRINTFUNC__;

	GtkWidget				*box;
	GtkWidget				*label, *inputUser, *inputPasswd;
	GtkWidget				*sPasswd;
	GtkEntryBuffer			*user, *passwd;
	int						line = 0;

	user = gtk_entry_buffer_new(NULL, -1);
	passwd = gtk_entry_buffer_new(NULL, -1);

	box = gtk_grid_new();

	label = gtk_label_new(_("User"));
	gtk_widget_set_margin_start(label, 12);
	gtk_widget_set_margin_end(label, 12);
	gtk_widget_set_margin_top(label, 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(box), label, 0, line, 1, 1);
	inputUser = gtk_entry_new_with_buffer(user);
	gtk_widget_set_hexpand(inputUser, TRUE);
	g_object_set_data(G_OBJECT(box),"userEntry", user);
	gtk_grid_attach(GTK_GRID(box), inputUser, 1, line++, 1, 1);


	label = gtk_label_new(_("Password"));
	gtk_widget_set_margin_start(label, 12);
	gtk_widget_set_margin_end(label, 12);
	gtk_widget_set_margin_top(label, 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(box), label, 0, line, 1, 1);
	inputPasswd = gtk_entry_new_with_buffer(passwd);
	gtk_entry_set_visibility(GTK_ENTRY(inputPasswd), FALSE);
	g_object_set_data(G_OBJECT(box),"passwdEntry", passwd);
	gtk_grid_attach(GTK_GRID(box), inputPasswd, 1, line++, 1, 1);

	sPasswd = gtk_check_button_new_with_label(_("Safe password?"));
	gtk_widget_set_margin_start(sPasswd, 12);
	gtk_widget_set_margin_end(sPasswd, 12);
	gtk_widget_set_margin_top(sPasswd, 6);
	gtk_widget_set_halign(GTK_WIDGET(sPasswd), GTK_ALIGN_START);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sPasswd), TRUE);
	g_object_set_data(G_OBJECT(box),"sPasswd", sPasswd);
	gtk_grid_attach(GTK_GRID(box), sPasswd, 1, line++, 1, 1);

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
	__PRINTFUNC__;

	GtkWidget				*box;
	GtkWidget				*label, *inputDesc, *inputUrl;
	GtkEntryBuffer			*desc, *url;
	int						line = 0;

	desc = gtk_entry_buffer_new(NULL, -1);
	url = gtk_entry_buffer_new(NULL, -1);

	box = gtk_grid_new();

	label = gtk_label_new(_("Description"));
	gtk_widget_set_margin_start(label, 12);
	gtk_widget_set_margin_end(label, 12);
	gtk_widget_set_margin_top(label, 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(box), label, 0, line, 1, 1);
	inputDesc = gtk_entry_new_with_buffer(desc);
	gtk_widget_set_hexpand(inputDesc, TRUE);
	g_object_set_data(G_OBJECT(box),"descEntry", desc);
	gtk_grid_attach(GTK_GRID(box), inputDesc, 1, line++, 1, 1);

	label = gtk_label_new(_("URL"));
	gtk_widget_set_margin_start(label, 12);
	gtk_widget_set_margin_end(label, 12);
	gtk_widget_set_margin_top(label, 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(box), label, 0, line, 1, 1);
	inputUrl = gtk_entry_new_with_buffer(url);
	gtk_widget_set_hexpand(inputUrl, TRUE);
	g_object_set_data(G_OBJECT(box),"urlEntry", url);
	gtk_grid_attach(GTK_GRID(box), inputUrl, 1, line++, 1, 1);

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
	__PRINTFUNC__;

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
	__PRINTFUNC__;

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
	__PRINTFUNC__;

	GtkWidget 			*assistant = NULL;
	GError				*error = NULL;
	GdkPixbuf			*pixbuf;

	assistant = gtk_assistant_new ();
	gtk_window_set_destroy_with_parent(GTK_WINDOW(assistant), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (assistant), 610, 377);

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
void syncMenuSel(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GError		 				*error = NULL;
	GThread						*thread;

	verboseCC("[%s] you selected %d\n", __func__, GPOINTER_TO_INT(trans));
	while(g_mutex_trylock(&mutex) != TRUE){}
    verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);
	thread = g_thread_try_new("syncingServer", syncOneServer, trans, &error);
	if(error){
		verboseCC("[%s] something has gone wrong with threads\n", __func__);
		verboseCC("%s\n", error->message);
		g_mutex_unlock(&mutex);
        verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
	}
	g_thread_unref(thread);
    verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
}

/**
 * syncMenuItem - one item of the sync menu
 */
static GtkWidget *syncMenuItem(int sID){
	__PRINTFUNC__;

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
	__PRINTFUNC__;

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
 * prefSrvDel - delete a server in the preferences dialog
 */
void prefSrvDel(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

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
 * prefSrvSave - save changes to a server in the preferences dialog
 */
void prefSrvSave(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	ContactCards_pref_t		*buffers = trans;
	GdkRGBA					rgba;
	int						old = -1;
	int						state = -1;

	if(buffers->srvID == 0){
		verboseCC("[%s] this isn't a server\n", __func__);
		return;
	}

	if(validateUrl((char *)gtk_entry_buffer_get_text(buffers->urlBuf)) == FALSE){
		feedbackDialog(GTK_MESSAGE_ERROR, _("Seems like that is not a valid URL"));
		return;
	}

	old = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", buffers->srvID, "", "", "", "");
	state = gtk_switch_get_active(GTK_SWITCH(buffers->syncSel));

	if((state == FALSE) && (old & CONTACTCARDS_ONE_WAY_SYNC) == CONTACTCARDS_ONE_WAY_SYNC){
		feedbackDialog(GTK_MESSAGE_INFO, _("Be careful when handling the uploads of contact!"));
	}

	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(buffers->colorChooser), &rgba);

	updateAddressbooks(appBase.db, buffers->aBooks);
	updateServerDetails(appBase.db, buffers->srvID,
						gtk_entry_buffer_get_text(buffers->descBuf), gtk_entry_buffer_get_text(buffers->urlBuf), gtk_entry_buffer_get_text(buffers->userBuf), gtk_entry_buffer_get_text(buffers->passwdBuf),
						gtk_switch_get_active(GTK_SWITCH(buffers->certSel)),
						gtk_switch_get_active(GTK_SWITCH(buffers->syncSel)),
						gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buffers->sPasswd)),
						gdk_rgba_to_string(&rgba));
	addressbookTreeUpdate();
}

#ifdef _USE_DANE
/**
 * prefServerCheckDane - Callback to check DANE/TLSA
 */
void prefServerCheckDane(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	ContactCards_pref_t			*buffers = trans;
	int							serverID = 0;

	if(buffers->srvID == 0){
		verboseCC("[%s] this isn't a server\n", __func__);
		return;
	}

	serverID = buffers->srvID;

	if(validateDANE(serverID) == TRUE){
		setSingleInt(appBase.db, "certs", "trustFlag", (int) ContactCards_DIGEST_TRUSTED | ContactCards_DANE, "serverID", serverID);
	}

}
#endif	/*	_USE_DANE	*/

/**
 * prefServerCheck - checking for address books
 */
void prefServerCheck(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

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
    verboseCC("%s():%d\tlocked mutex\n", __func__, __LINE__);
	isOAuth = getSingleInt(appBase.db, "cardServer", "isOAuth", 1, "serverID", buffers->srvID, "", "", "", "");

	if(isOAuth){
		int		ret = 0;
		ret = oAuthUpdate(appBase.db, buffers->srvID);
		if(ret != OAUTH_UP2DATE){
			g_mutex_unlock(&mutex);
            verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);
			return;
		}
	}

	sess = serverConnect(buffers->srvID);
	syncInitial(appBase.db, sess, buffers->srvID);
	serverDisconnect(sess, appBase.db, buffers->srvID);

	g_mutex_unlock(&mutex);
    verboseCC("%s():%d\tunlocked mutex\n", __func__, __LINE__);

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
	__PRINTFUNC__;

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
 * prefSrvSel - select a server in the preferences dialog
 */
void prefSrvSel(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GtkTreeIter			iter;
	GtkTreeModel		*model;
	GtkWidget			*prefFrame;
	GList               *children, *iter2;
	int					selID;
	GSList				*abList;
	ContactCards_pref_t		*buffers = trans;
	char				*frameTitle = NULL, *user = NULL, *passwd = NULL, *synctime = NULL;
	char				*issued = NULL, *issuer = NULL, *url = NULL, *color = NULL;
	int					isOAuth;
	int					flags = 0;
	gboolean			res = 0;
	GdkRGBA				rgba;

	prefFrame = buffers->prefFrame;

    if(GTK_IS_TREE_SELECTION(widget)){
        verboseCC("is tree selection\n");
        if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
            gtk_tree_model_get(model, &iter, ID_COLUMN, &selID,  -1);
        } else {
            debugCC("[%s] Failed to get selection", __func__);
            return;
        }
    } else {
        selID = getSingleIntRand(appBase.db, "cardServer", "serverID");
        if (selID < 1){
            debugCC("[%s] Failed to get a random selection", __func__);
            return;
        }
    }

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
		flags = getSingleInt(appBase.db, "cardServer", "flags", 1, "serverID", selID, "", "", "", "");
		if(flags & CONTACTCARDS_NO_PASSWD){
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buffers->sPasswd), FALSE);
		} else {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buffers->sPasswd), TRUE);
		}
	} else {
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->passwdBuf), "", -1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buffers->sPasswd), FALSE);
	}

    synctime = getSingleChar(appBase.db, "cardServer", "lastSynced", 1, "serverID", selID, "", "", "", "", "", 0);
    gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->lastsynced), synctime, -1);

	url = getSingleChar(appBase.db, "cardServer", "srvUrl", 1, "serverID", selID, "", "", "", "", "", 0);
	gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->urlBuf), url, -1);

	gtk_switch_set_active(GTK_SWITCH(buffers->certSel), FALSE);

	color = getSingleChar(appBase.db, "cardServer", "color", 1, "serverID", selID, "", "", "", "", "", 0);
	if(gdk_rgba_parse(&rgba, color) == TRUE)
		gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(buffers->colorChooser), &rgba);
	free(color);

	if(countElements(appBase.db, "certs", 1, "serverID", selID, "", "", "", "") == 1){

		issued = getSingleChar(appBase.db, "certs", "issued", 1, "serverID", selID, "", "", "", "", "", 0);
		if(issued == NULL) issued = "";
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuedBuf), issued, -1);

		issuer = getSingleChar(appBase.db, "certs", "issuer", 1, "serverID", selID, "", "", "", "", "", 0);
		if(issuer == NULL) issuer = "";
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(buffers->issuerBuf), issuer, -1);

		res = getSingleInt(appBase.db, "certs", "trustFlag", 1, "serverID", selID, "", "", "", "");
		if((res & ContactCards_DIGEST_TRUSTED) == ContactCards_DIGEST_TRUSTED){
			gtk_switch_set_active(GTK_SWITCH(buffers->certSel), TRUE);
		} else {
			gtk_switch_set_active(GTK_SWITCH(buffers->certSel), FALSE);
		}
#ifdef _USE_DANE
		if((res & ContactCards_DANE) == ContactCards_DANE){
			gtk_label_set_text(GTK_LABEL(buffers->dane), _("Certificate seems to be trustworthy."));
		} else {
			gtk_label_set_text(GTK_LABEL(buffers->dane), _("No information available via DANE."));
		}
#endif
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
	__PRINTFUNC__;

	g_free(data);
}

/**
 * prefKeyHandler - control some kind of the preferences dialog by keyboard
 */
void prefKeyHandler(GtkWidget *window, GdkEventKey *event, gpointer data){
	__PRINTFUNC__;

	if (event->keyval == GDK_KEY_w && (event->state & GDK_CONTROL_MASK)) {
		gtk_widget_destroy(window);
	}
}

/**
 * prefGenSave - Save the general settings
 */
void prefGenSave(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	ContactCards_genPref_t		*gen = trans;
	int							newSort, newMap, newInterval, newLocal;
	int							newFlag = 0;

	/*	Delete old flags	*/
	appBase.flags &= ~DISPLAY_STYLE_MASK;
	appBase.flags &= ~USE_MAP_MASK;
	appBase.flags &= ~CONTACTCARDS_NO_LOCAL;

	newSort	= gtk_combo_box_get_active(GTK_COMBO_BOX(gen->sort));
	newMap	= gtk_combo_box_get_active(GTK_COMBO_BOX(gen->map));
	newInterval = gtk_combo_box_get_active(GTK_COMBO_BOX(gen->interval));
	newLocal = gtk_combo_box_get_active(GTK_COMBO_BOX(gen->locals));

	debugCC("\t\tstyle:%d\tmap: %d\tlocal: %d\n", newSort, newMap, newLocal);

	switch(newLocal){
		case 1:
			newFlag |= CONTACTCARDS_NO_LOCAL;
			break;
		default:
			/*	Use a local adress book	*/
			break;
	}

	switch(newSort){
		case 0:
			newFlag |= FAMILYNAME_FIRST;
			break;
		case 1:
			newFlag |= GIVENNAME_FIRST;
			break;
		case 2:
			newFlag |= FAMILYNAME_ONLY;
			break;
		default:
			newFlag |= FAMILYNAME_FIRST;
			break;
	}

	switch(newMap){
		case 0:
			newFlag |= USE_OSM;
			break;
		case 1:
			newFlag |= USE_GOOGLE;
			break;
		default:
			newFlag |= USE_OSM;
			break;
	}

	appBase.flags |= newFlag;

	switch(newInterval){
		case 0:
			appBase.syncIntervall = 3600;
			break;
		case 1:
			appBase.syncIntervall = 3600 * 2;
			break;
		case 2:
			appBase.syncIntervall = 3600 * 4;
			break;
		case 3:
			appBase.syncIntervall = 3600 * 8;
			break;
		default:
			appBase.syncIntervall = 3600 * 2;
			break;
	}
}

/**
 * prefViewGen - Dialogview for general settings
 */
void prefViewGen(GtkWidget *btn, gpointer *trans){
	__PRINTFUNC__;

	GtkWidget			*layout = GTK_WIDGET(trans);
	GtkWidget			*prefView;
	GtkWidget			*txt;
	GtkWidget			*sort, *map, *interval, *localBook;
	GtkWidget			*sBtn;
	int					line = 2;
	ContactCards_genPref_t		*gen;

	viewCleaner(layout);
	prefView = gtk_grid_new();

	gen = g_new(ContactCards_genPref_t, 1);

	gtk_widget_set_hexpand(GTK_WIDGET(prefView), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(prefView), TRUE);
	gtk_widget_set_halign(GTK_WIDGET(prefView), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(prefView), GTK_ALIGN_START);

	txt = gtk_label_new(_("Sorting"));
	gtk_widget_set_margin_start(txt, 12);
	gtk_widget_set_margin_end(txt, 12);
	gtk_widget_set_margin_top(txt, 18);
	gtk_widget_set_halign(txt, GTK_ALIGN_END);
	sort = gtk_combo_box_text_new();
	gtk_widget_set_margin_top(sort, 18);
	gtk_widget_set_halign(sort, GTK_ALIGN_START);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sort), "0x1", _("Last name first"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sort), "0x2", _("Given name first"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sort), "0x4", _("Last name only"));
	if((appBase.flags & FAMILYNAME_FIRST) == FAMILYNAME_FIRST)
		gtk_combo_box_set_active(GTK_COMBO_BOX(sort), 0);
	else if((appBase.flags & GIVENNAME_FIRST) == GIVENNAME_FIRST)
		gtk_combo_box_set_active(GTK_COMBO_BOX(sort), 1);
	else if((appBase.flags & FAMILYNAME_ONLY) == FAMILYNAME_ONLY)
		gtk_combo_box_set_active(GTK_COMBO_BOX(sort), 2);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(sort), 0);
	gtk_grid_attach(GTK_GRID(prefView), txt, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(prefView), sort, 1, line++, 2, 1);
	line++;

	txt = gtk_label_new(_("Map to use"));
	gtk_widget_set_margin_start(txt, 12);
	gtk_widget_set_margin_end(txt, 12);
	gtk_widget_set_margin_top(txt, 18);
	gtk_widget_set_halign(txt, GTK_ALIGN_END);
	map = gtk_combo_box_text_new();
	gtk_widget_set_margin_top(map, 18);
	gtk_widget_set_halign(map, GTK_ALIGN_START);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(map), "16", _("Open Street Maps"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(map), "32", _("Google Maps"));

	if((appBase.flags & USE_OSM) == USE_OSM)
		gtk_combo_box_set_active(GTK_COMBO_BOX(map), 0);
	else if((appBase.flags & USE_GOOGLE) == USE_GOOGLE)
		gtk_combo_box_set_active(GTK_COMBO_BOX(map), 1);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(map), 0);
	gtk_grid_attach(GTK_GRID(prefView), txt, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(prefView), map, 1, line++, 2, 1);
	line++;

	txt = gtk_label_new(_("Sync interval"));
	gtk_widget_set_margin_start(txt, 12);
	gtk_widget_set_margin_end(txt, 12);
	gtk_widget_set_margin_top(txt, 18);
	gtk_widget_set_halign(txt, GTK_ALIGN_END);
	interval = gtk_combo_box_text_new();
	gtk_widget_set_margin_top(interval, 18);
	gtk_widget_set_halign(interval, GTK_ALIGN_START);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(interval), "1", _("1 h"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(interval), "2", _("2 h"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(interval), "4", _("4 h"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(interval), "8", _("8 h"));

	switch((appBase.syncIntervall / 3600)){
		case 1:
			gtk_combo_box_set_active(GTK_COMBO_BOX(interval), 0);
			break;
		case 2:
			gtk_combo_box_set_active(GTK_COMBO_BOX(interval), 1);
			break;
		case 4:
			gtk_combo_box_set_active(GTK_COMBO_BOX(interval), 2);
			break;
		case 8:
			gtk_combo_box_set_active(GTK_COMBO_BOX(interval), 3);
			break;
		default:
			gtk_combo_box_set_active(GTK_COMBO_BOX(interval), 1);
			break;
	}
	gtk_grid_attach(GTK_GRID(prefView), txt, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(prefView), interval, 1, line++, 2, 1);
	line++;

	txt = gtk_label_new(_("Use local address book?"));
	gtk_widget_set_margin_start(txt, 12);
	gtk_widget_set_margin_end(txt, 12);
	gtk_widget_set_margin_top(txt, 18);
	gtk_widget_set_halign(txt, GTK_ALIGN_END);
	localBook = gtk_combo_box_text_new();
	gtk_widget_set_margin_top(localBook, 18);
	gtk_widget_set_halign(localBook, GTK_ALIGN_START);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(localBook), "1", _("Yes"));
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(localBook), "2", _("No"));
	if((appBase.flags & CONTACTCARDS_NO_LOCAL) == CONTACTCARDS_NO_LOCAL){
		gtk_combo_box_set_active(GTK_COMBO_BOX(localBook), 1);
	} else {
		gtk_combo_box_set_active(GTK_COMBO_BOX(localBook), 0);
	}
	gtk_grid_attach(GTK_GRID(prefView), txt, 0, line, 1, 1);
	gtk_grid_attach(GTK_GRID(prefView), localBook, 1, line++, 2, 1);
	line++;

	gen->sort		= sort;
	gen->map		= map;
	gen->interval	= interval;
	gen->locals		= localBook;

	sBtn = gtk_button_new_with_label(_("Save changes"));
	gtk_widget_set_margin_start(sBtn, 12);
	gtk_widget_set_margin_end(sBtn, 12);
	gtk_widget_set_margin_top(sBtn, 18);
	gtk_widget_set_halign(sBtn, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(prefView), sBtn, 4, line++, 1, 1);
	g_signal_connect(sBtn, "clicked", G_CALLBACK(prefGenSave), gen);

	g_signal_connect(prefView, "unmap", G_CALLBACK(prefExit), gen);

	gtk_widget_show_all(prefView);
	gtk_container_add(GTK_CONTAINER(layout), prefView);
}

/**
 * prefViewSrv - Dialogview for the server settings
 */
void prefViewSrv(GtkWidget *btn, gpointer *trans){
	__PRINTFUNC__;

	GtkWidget			*layout = GTK_WIDGET(trans);
	GtkWidget			*prefView, *prefFrame, *prefList;
	GtkWidget			*label, *input;
	GtkWidget			*srvList, *scroll, *box, *abbox;
	GtkWidget			*digSwitch, *uploadSwitch;
	GtkWidget			*saveBtn, *deleteBtn, *exportCertBtn, *checkBtn;
	GtkWidget			*sPasswd;
	GtkWidget			*ablist, *colorBtn;
	GSList				*aBooks = g_slist_alloc();
	GtkEntryBuffer		*desc, *url, *user, *passwd, *timestr;
	GtkEntryBuffer		*issued, *issuer;
	GtkTreeSelection	*srvSel;
	int					line = 1;
#ifdef _USE_DANE
	GtkWidget			*daneResult;
	GtkWidget			*daneCheck;
#endif	/*	_USE_DANE	*/

	ContactCards_pref_t		*buffers = NULL;

	desc = gtk_entry_buffer_new(NULL, -1);
	url = gtk_entry_buffer_new(NULL, -1);
	user = gtk_entry_buffer_new(NULL, -1);
	passwd = gtk_entry_buffer_new(NULL, -1);
	issued = gtk_entry_buffer_new(NULL, -1);
	issuer = gtk_entry_buffer_new(NULL, -1);
    timestr = gtk_entry_buffer_new(NULL, -1);

	buffers = g_new(ContactCards_pref_t, 1);
	prefView = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	viewCleaner(layout);

	prefList = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(prefList), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(prefList, 128, -1);

	srvList = gtk_tree_view_new();
	listInit(srvList);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(srvList), FALSE);
	gtk_container_add(GTK_CONTAINER(prefList), srvList);
	fillList(appBase.db, 3, 0, 0, srvList);

	prefFrame = gtk_frame_new(_("Settings"));
	gtk_widget_set_margin_top(GTK_WIDGET(prefFrame), 3);
	gtk_widget_set_margin_bottom(GTK_WIDGET(prefFrame), 3);
	gtk_widget_set_margin_start(GTK_WIDGET(prefFrame), 3);
	gtk_widget_set_margin_end(GTK_WIDGET(prefFrame), 3);

    scroll = gtk_scrolled_window_new(NULL, NULL);
	box = gtk_grid_new();

	label = gtk_label_new(_("Description"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	input = gtk_entry_new_with_buffer(desc);
	gtk_widget_set_margin_top(GTK_WIDGET(input), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(input), 3);
	gtk_grid_attach(GTK_GRID(box), input, 3, line++, 4, 1);

	label = gtk_label_new(_("URL"));
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	input = gtk_entry_new_with_buffer(url);
	gtk_widget_set_margin_start(GTK_WIDGET(input), 3);
	gtk_grid_attach(GTK_GRID(box), input, 3, line++, 4, 1);

	line++;

	label = gtk_label_new(_("User"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
    gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	input = gtk_entry_new_with_buffer(user);
	gtk_widget_set_margin_top(GTK_WIDGET(input), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(input), 3);
	gtk_grid_attach(GTK_GRID(box), input, 3, line++, 4, 1);

	label = gtk_label_new(_("Password"));
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	input = gtk_entry_new_with_buffer(passwd);
	gtk_entry_set_visibility(GTK_ENTRY(input), FALSE);
	gtk_widget_set_margin_start(GTK_WIDGET(input), 3);
	gtk_grid_attach(GTK_GRID(box), input, 3, line++, 4, 1);

	sPasswd = gtk_check_button_new_with_label(_("Safe password?"));
	gtk_widget_set_margin_top(GTK_WIDGET(sPasswd), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(sPasswd), 3);
	gtk_grid_attach(GTK_GRID(box), sPasswd, 3, line++, 4, 1);

	line++;

    label = gtk_label_new(_("Last time synced"));
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	input = gtk_entry_new_with_buffer(timestr);
	gtk_widget_set_margin_start(GTK_WIDGET(input), 3);
    gtk_editable_set_editable(GTK_EDITABLE(input), FALSE);
	gtk_grid_attach(GTK_GRID(box), input, 3, line++, 4, 1);

    line++;

	label = gtk_label_new(_("Color"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	colorBtn = gtk_color_button_new();
	gtk_widget_set_margin_top(GTK_WIDGET(colorBtn), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(colorBtn), 3);
	gtk_grid_attach(GTK_GRID(box), colorBtn, 3, line++, 1, 1);

	line++;

	label = gtk_label_new(_("Certificate is issued for"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	input = gtk_entry_new_with_buffer(issued);
	gtk_widget_set_margin_top(GTK_WIDGET(input), 6);
	gtk_editable_set_editable(GTK_EDITABLE(input), FALSE);
	gtk_widget_set_margin_start(GTK_WIDGET(input), 3);
	gtk_grid_attach(GTK_GRID(box), input, 3, line++, 3, 1);

	label = gtk_label_new(_("Certificate issued by"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	input = gtk_entry_new_with_buffer(issuer);
	gtk_widget_set_margin_top(GTK_WIDGET(input), 6);
	gtk_editable_set_editable(GTK_EDITABLE(input), FALSE);
	gtk_widget_set_margin_start(GTK_WIDGET(input), 3);
	gtk_grid_attach(GTK_GRID(box), input, 3, line++, 3, 1);

	line++;

	exportCertBtn = gtk_button_new_with_label(_("Export Certificate"));
	gtk_widget_set_margin_top(GTK_WIDGET(exportCertBtn), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(exportCertBtn), 3);
	gtk_grid_attach(GTK_GRID(box), exportCertBtn, 3, line++, 2, 1);

#ifdef _USE_DANE
	line++;
	label = gtk_label_new(_("DANE/TLSA"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	daneResult = gtk_label_new("");
	gtk_widget_set_margin_top(GTK_WIDGET(daneResult), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(daneResult), 3);
	gtk_grid_attach(GTK_GRID(box), daneResult, 3, line++, 3, 1);
	line++;
	daneCheck = gtk_button_new_with_label(_("Check DANE/TLSA again"));
	gtk_widget_set_margin_top(GTK_WIDGET(daneCheck), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(daneCheck), 3);
	gtk_grid_attach(GTK_GRID(box), daneCheck, 3, line++, 2, 1);
	line++;
#endif		/*		_USE_DANE		*/

	label = gtk_label_new(_("Trust Certificate?"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	digSwitch = gtk_switch_new();
	gtk_widget_set_margin_top(GTK_WIDGET(digSwitch), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(digSwitch), 3);
	gtk_grid_attach(GTK_GRID(box), digSwitch, 3, line++, 1, 1);

	line++;

	label = gtk_label_new(_("One-Way-Sync"));
	gtk_widget_set_margin_top(GTK_WIDGET(label), 6);
	gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_END);
	gtk_widget_set_margin_start(GTK_WIDGET(label), 3);
	gtk_grid_attach(GTK_GRID(box), label, 2, line, 1, 1);
	uploadSwitch = gtk_switch_new();
	gtk_widget_set_margin_top(GTK_WIDGET(uploadSwitch), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(uploadSwitch), 3);
	gtk_grid_attach(GTK_GRID(box), uploadSwitch, 3, line++, 1, 1);

	line++;

	checkBtn = gtk_button_new_with_label(_("Check for address books"));
	gtk_widget_set_margin_top(GTK_WIDGET(checkBtn), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(checkBtn), 3);
	gtk_grid_attach(GTK_GRID(box), checkBtn, 3, line++, 2, 1);

	line++;

	abbox = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(abbox), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(abbox, 256, 128);
	ablist = gtk_list_box_new();
	gtk_container_add(GTK_CONTAINER(abbox), ablist);
	gtk_widget_set_margin_top(GTK_WIDGET(abbox), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(abbox), 3);
	gtk_grid_attach(GTK_GRID(box), abbox, 3, line++, 4, 1);

	line++;

	deleteBtn = gtk_button_new_with_label(_("Remove server"));
	gtk_widget_set_margin_top(GTK_WIDGET(deleteBtn), 6);
	gtk_widget_set_margin_bottom(GTK_WIDGET(deleteBtn), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(deleteBtn), 3);
	gtk_grid_attach(GTK_GRID(box), deleteBtn, 4, line, 1, 1);
	saveBtn = gtk_button_new_with_label(_("Save changes"));
	gtk_widget_set_margin_top(GTK_WIDGET(saveBtn), 6);
	gtk_widget_set_margin_bottom(GTK_WIDGET(saveBtn), 6);
	gtk_widget_set_margin_start(GTK_WIDGET(saveBtn), 3);
	gtk_grid_attach(GTK_GRID(box), saveBtn, 5, line++, 1, 1);

	gtk_container_add(GTK_CONTAINER(scroll), box);
	gtk_container_add(GTK_CONTAINER(prefFrame), scroll);

	buffers->prefFrame = prefFrame;
	buffers->descBuf = desc;
	buffers->urlBuf = url;
	buffers->userBuf = user;
	buffers->passwdBuf = passwd;
	buffers->issuedBuf = issued;
	buffers->issuerBuf = issuer;
    buffers->lastsynced = timestr;
	buffers->srvPrefList = srvList;
	buffers->certSel = digSwitch;
	buffers->syncSel = uploadSwitch;
	buffers->listbox = ablist;
	buffers->aBooks = aBooks;
	buffers->colorChooser = colorBtn;
	buffers->sPasswd = sPasswd;
#ifdef _USE_DANE
	buffers->dane = daneResult;
#endif	/*	_USE_DANE	*/

	/*		Connect Signales		*/
	srvSel = gtk_tree_view_get_selection(GTK_TREE_VIEW(srvList));
	gtk_tree_selection_set_mode (srvSel, GTK_SELECTION_SINGLE);
	g_signal_connect(srvSel, "changed", G_CALLBACK(prefSrvSel), buffers);

	g_signal_connect(prefView, "unmap", G_CALLBACK(prefExit), buffers);

	g_signal_connect(deleteBtn, "clicked", G_CALLBACK(prefSrvDel), buffers);
	g_signal_connect(saveBtn, "clicked", G_CALLBACK(prefSrvSave), buffers);
	g_signal_connect(exportCertBtn, "clicked", G_CALLBACK(prefExportCert), buffers);
	g_signal_connect(checkBtn, "clicked", G_CALLBACK(prefServerCheck), buffers);
#ifdef _USE_DANE
	g_signal_connect(daneCheck, "clicked", G_CALLBACK(prefServerCheckDane), buffers);
#endif	/*	_USE_DANE	*/
    g_signal_connect(prefView, "show", G_CALLBACK(prefSrvSel), buffers);

	gtk_container_add(GTK_CONTAINER(prefView), prefList);
	gtk_container_add(GTK_CONTAINER(prefView), prefFrame);
	gtk_widget_show_all(prefView);
	gtk_container_add(GTK_CONTAINER(layout), prefView);
}

/**
 * prefWindow - build the preferences dialog
 */
void prefWindow(GtkWidget *widget, gpointer trans){
	__PRINTFUNC__;

	GtkWidget			*prefWindow, *prefView;
	GtkWidget			*prefToolBar;
	GtkWidget			*prefLayout;
	GtkWidget			*empty;
	GtkToolItem			*prefGen, *prefSrv;
	GError				*error = NULL;
	GdkPixbuf			*pixbuf;

	prefWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(prefWindow), _("Preferences"));
	gtk_window_resize(GTK_WINDOW(prefWindow), 665, 521);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(prefWindow), TRUE);

	pixbuf = gdk_pixbuf_new_from_file("artwork/icon_48.png", &error);
	if(error){
		verboseCC("[%s] something has gone wrong\n", __func__);
		verboseCC("%s\n", error->message);
	}
	gtk_window_set_icon(GTK_WINDOW(prefWindow), pixbuf);
	g_object_unref(pixbuf);

	prefLayout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	prefToolBar = gtk_toolbar_new();
	prefGen = gtk_tool_button_new(NULL, _("General settings"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(prefGen), _("General settings"));
	gtk_toolbar_insert(GTK_TOOLBAR(prefToolBar), prefGen, -1);
	prefSrv = gtk_tool_button_new(NULL, _("Server settings"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(prefSrv), _("Server settings"));
	gtk_toolbar_insert(GTK_TOOLBAR(prefToolBar), prefSrv, -1);
	gtk_box_pack_start(GTK_BOX(prefLayout), GTK_WIDGET(prefToolBar), FALSE, FALSE, 2);

	prefView = gtk_frame_new (NULL);
	gtk_box_pack_start(GTK_BOX(prefLayout), GTK_WIDGET(prefView), TRUE, TRUE, 2);

	empty = gtk_image_new_from_icon_name("preferences-system-symbolic", GTK_ICON_SIZE_DIALOG);
	gtk_container_add(GTK_CONTAINER(prefView), empty);

	g_signal_connect(prefGen, "clicked", G_CALLBACK(prefViewGen), prefView);
	g_signal_connect(prefSrv, "clicked", G_CALLBACK(prefViewSrv), prefView);

	g_signal_connect(G_OBJECT(prefWindow), "key_press_event", G_CALLBACK(prefKeyHandler), NULL);

    g_signal_connect(prefWindow, "show", G_CALLBACK(prefViewGen), prefView);

	gtk_container_add(GTK_CONTAINER(prefWindow), prefLayout);
	gtk_widget_show_all(prefWindow);
}

/**
 * syncMenuFill - fills the menu with items
 */
static void syncMenuFill(void){
	__PRINTFUNC__;

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
	__PRINTFUNC__;

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
	__PRINTFUNC__;

	syncMenuFlush();
	syncMenuFill();
}

/**
 * birthdayListAppend - appends the birthday to the list for the tooltips
 */
void birthdayListAppend(int day, char *card){
	__PRINTFUNC__;

	ContactCards_cal_item_t			*item;
	char							*displayname;

	if(g_slist_length(appBase.callist) == 1){
		goto insertDirect;
	}

	while(appBase.callist){
		GSList						*next = (appBase.callist)->next;
		ContactCards_cal_item_t		*item = (appBase.callist)->data;
		if(!item){
			goto stepForward;
		}
		if(item->day == day){
			char			*old = item->txt;
			char			*append = getSingleCardAttribut(CARDTYPE_FN, card);
			char			*new = NULL;

			if(strlen(g_strstrip(append)) == 0)
				append = g_strndup("(no name)", sizeof("(no name)"));

			new = g_strconcat(old, "\n", append, NULL);
			debugCC("\t\tappending %s\n", new);
			item->txt = new;
			return;
		}
stepForward:
		(appBase.callist) = next;
	}

insertDirect:
	item = g_new(ContactCards_cal_item_t, 1);
	item->day = day;
	displayname = getSingleCardAttribut(CARDTYPE_FN, card);
	if(strlen(g_strstrip(displayname)) == 0)
		displayname = g_strndup("(no name)", sizeof("(no name)"));
	item->txt = displayname;
	appBase.callist = g_slist_append((appBase.callist), item);
}

/**
 * markDay - mark the birthday of each contact in the calendar
 */
void markDay(GSList *contacts){
	__PRINTFUNC__;

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
		if((flags & CONTACTCARDS_TMP) == CONTACTCARDS_TMP){
			debugCC("[%s] hiding %d\n", __func__, id);
			contacts = next;
			continue;
		}

		card = getSingleChar(appBase.db, "contacts", "vCard", 1, "contactID", id, "", "", "", "", "", 0);

		bday = getSingleCardAttribut(CARDTYPE_BDAY, card);
		if(bday != NULL){
			GDate				*date;
			unsigned int		month;
			date = g_date_new();
			g_date_set_parse(date, bday);
			if(g_date_valid(date) == TRUE){
				gtk_calendar_get_date(GTK_CALENDAR(appBase.cal), NULL, &month, NULL);
				if(g_date_get_month(date) == (month+1)){
					gtk_calendar_mark_day(GTK_CALENDAR(appBase.cal), (int)g_date_get_day(date));
					birthdayListAppend(g_date_get_day(date), card);
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
void calendarUpdate(int type, int id){
	__PRINTFUNC__;

	GSList			*contacts = NULL;

	/*	Clean up at first	*/
	gtk_calendar_clear_marks (GTK_CALENDAR(appBase.cal));

	if (g_slist_length (appBase.callist) > 1){
		debugCC("Delete old list and create a new one\n");
		g_slist_free_full(appBase.callist, g_free);
		appBase.callist = g_slist_alloc();
	}

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
					markDay(contacts);
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
		case 2:
			contacts = getListInt(appBase.db, "contacts", "contactID", 91, "flags", CONTACTCARDS_FAVORIT, "", "", "", "");
			break;
		case 3:
			contacts = getListInt(appBase.db, "contacts", "contactID", 91, "flags", CONTACTCARDS_LOCAL, "", "", "", "");
			break;
		default:
			break;
	}
	markDay(contacts);
	g_slist_free(contacts);

}

/**
 * birthdayTooltip - Adds an tooltip to the calender
 */
gchar *birthdayTooltip(GtkCalendar *cal, guint year, guint month, guint day, gpointer trans){
	__PRINTFUNC__;

	GSList							*list = appBase.callist;

	if(g_slist_length(list) == 1)
		return NULL;

	while(list){
		GSList						*next = list->next;
		ContactCards_cal_item_t		*cmp;
		if(!list->data){
			goto stepForward;
		}
		cmp = list->data;
		if(cmp->day == day){
			return g_strdup(cmp->txt);
		}
stepForward:
		list = next;
	}

	return NULL;
}
