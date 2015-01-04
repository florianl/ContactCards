/* Copyright (c) 2013-2015 Florian L. <dev@der-flo.net>
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

static char			*alternate_config = NULL;
static char			*version = NULL;
static gboolean		verbose = FALSE;
static gboolean		debug = FALSE;

static GOptionEntry entries[] =
{
	{ "config", 'c', 0, G_OPTION_ARG_FILENAME, &alternate_config, "Alternate configuration directory", NULL },
	{ "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "debugging output", NULL },
	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "verbose output", NULL },
	{ "version", 'V', 0, G_OPTION_ARG_NONE, &version, "Show version", NULL },
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

ContactCards_app_t *parseCmdLine(int *argc, char **argv[]){
	__PRINTFUNC__;

	GError				*error = NULL;
	GOptionContext		*context;
	ContactCards_app_t	*app;

	context = g_option_context_new(_("ContactCards"));
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_parse(context, argc, argv, &error);
	g_option_context_free(context);

	if(error){
		verboseCC("%s\n", error->message);
		exit(EXIT_FAILURE);
	}

	if (version){
		printf("ContactCards %s\n", VERSION);
		exit(EXIT_SUCCESS);
	}

	app = g_new(ContactCards_app_t, 1);

	app->verbose = verbose;
	app->debug = debug;

	if(alternate_config){
		app->configdir = alternate_config;
	}else {
		app->configdir = g_build_filename(g_get_user_config_dir(), "contactcards", NULL);
	}

	return app;
}

static void checkConfigDir(char *dir){
	__PRINTFUNC__;

	if (!g_file_test(dir, G_FILE_TEST_EXISTS)){
		int			ret = 0;
		verboseCC("[%s] configuration directory doesn't exist yet\n", __func__);

		if (dir == NULL || strlen(dir) == 0)
			exit(EXIT_FAILURE);

		ret = g_mkdir_with_parents(dir, 0700);
		if(ret)
			verboseCC("[%s] failed to create the configuration directory with error %d\n", __func__, ret);
	}
}

void verboseCC(gchar const *format, ...){
	va_list args;
	va_start(args, format);
	g_logv("ContactCards", G_LOG_LEVEL_INFO, format, args);
	va_end(args);
}

void debugCC(gchar const *format, ...){
	va_list args;
	va_start(args, format);
	g_logv("ContactCards", G_LOG_LEVEL_DEBUG, format, args);
	va_end(args);
}

static void logHandler(const gchar *domain, GLogLevelFlags level, const gchar *msg, gpointer data){
	printf("[%s] %s", domain, msg);
}

static void configOutput(ContactCards_app_t *app){
	__PRINTFUNC__;

	if(app->verbose){
		g_log_set_handler("ContactCards", G_LOG_LEVEL_INFO, logHandler, NULL);
	}

	if(app->debug){
		g_log_set_handler("ContactCards", G_LOG_LEVEL_DEBUG, logHandler, NULL);
	}

}

void checkAndSetConfig(ContactCards_app_t *app){
	__PRINTFUNC__;

	checkConfigDir(app->configdir);
	configOutput(app);
}

static void config_load_ui(GKeyFile *config){
	__PRINTFUNC__;

	GError			*error = NULL;
	int				tmp = 0;
	int				flags = 0;
	int				syncIntervall = 0;

	tmp |= g_key_file_get_integer(config, PACKAGE, "formation", &error);
	if (error){
			verboseCC("%s\n", error->message);
			g_clear_error(&error);
	} else {
		flags |= tmp;
	}
	tmp = 0;
/*
	tmp |= g_key_file_get_integer(config, PACKAGE, "separator", &error);
	if (error){
			verboseCC("%s\n", error->message);
			g_clear_error(&error);
	} else {
		if(tmp == 1){
			flags |= 1 << USE_SEPARATOR;
		} else {
			flags |= 0 << USE_SEPARATOR;
		}
	}
	tmp = 0;
*/
	appBase.flags = flags;

	syncIntervall = g_key_file_get_integer(config, PACKAGE, "syncIntervall", &error);
	if (error){
			verboseCC("%s\n", error->message);
			g_clear_error(&error);
	}
	if(syncIntervall != 0){
		debugCC("Intervalltime: %d\n", syncIntervall);
		appBase.syncIntervall = syncIntervall;
	} else {
		appBase.syncIntervall = 1800;
	}

}

void config_load(ContactCards_app_t *app){
	__PRINTFUNC__;

	gchar			*configfile = g_build_filename(app->configdir, "contactcards.conf", NULL);
	GKeyFile		*config = g_key_file_new();

	if (! g_file_test(configfile, G_FILE_TEST_IS_REGULAR)){
		verboseCC("[%s] configuration file doesn't exist yet\n", __func__);
		/*	Set default value	*/
		appBase.syncIntervall = 1800;
		return;
	}
	g_key_file_load_from_file(config, configfile, G_KEY_FILE_NONE, NULL);
	g_free(configfile);

	config_load_ui(config);

	g_key_file_free(config);
}
