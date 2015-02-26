/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>

#include "fu-cleanup.h"
#include "fu-common.h"

#define FU_ERROR			1
#define FU_ERROR_INVALID_ARGUMENTS	0
#define FU_ERROR_NO_SUCH_CMD		1

typedef struct {
	GOptionContext		*context;
	GPtrArray		*cmd_array;
} FuUtilPrivate;

typedef gboolean (*FuUtilPrivateCb)	(FuUtilPrivate	*util,
					 gchar		**values,
					 GError		**error);

typedef struct {
	gchar		*name;
	gchar		*arguments;
	gchar		*description;
	FuUtilPrivateCb	 callback;
} FuUtilItem;

/**
 * fu_util_item_free:
 **/
static void
fu_util_item_free (FuUtilItem *item)
{
	g_free (item->name);
	g_free (item->arguments);
	g_free (item->description);
	g_free (item);
}

/*
 * fu_sort_command_name_cb:
 */
static gint
fu_sort_command_name_cb (FuUtilItem **item1, FuUtilItem **item2)
{
	return g_strcmp0 ((*item1)->name, (*item2)->name);
}

/**
 * fu_util_add:
 **/
static void
fu_util_add (GPtrArray *array,
	     const gchar *name,
	     const gchar *arguments,
	     const gchar *description,
	     FuUtilPrivateCb callback)
{
	guint i;
	FuUtilItem *item;
	_cleanup_strv_free_ gchar **names = NULL;

	g_return_if_fail (name != NULL);
	g_return_if_fail (description != NULL);
	g_return_if_fail (callback != NULL);

	/* add each one */
	names = g_strsplit (name, ",", -1);
	for (i = 0; names[i] != NULL; i++) {
		item = g_new0 (FuUtilItem, 1);
		item->name = g_strdup (names[i]);
		if (i == 0) {
			item->description = g_strdup (description);
		} else {
			/* TRANSLATORS: this is a command alias */
			item->description = g_strdup_printf (_("Alias to %s"),
							     names[0]);
		}
		item->arguments = g_strdup (arguments);
		item->callback = callback;
		g_ptr_array_add (array, item);
	}
}

/**
 * fu_util_get_descriptions:
 **/
static gchar *
fu_util_get_descriptions (GPtrArray *array)
{
	guint i;
	guint j;
	guint len;
	const guint max_len = 35;
	FuUtilItem *item;
	GString *string;

	/* print each command */
	string = g_string_new ("");
	for (i = 0; i < array->len; i++) {
		item = g_ptr_array_index (array, i);
		g_string_append (string, "  ");
		g_string_append (string, item->name);
		len = strlen (item->name) + 2;
		if (item->arguments != NULL) {
			g_string_append (string, " ");
			g_string_append (string, item->arguments);
			len += strlen (item->arguments) + 1;
		}
		if (len < max_len) {
			for (j = len; j < max_len + 1; j++)
				g_string_append_c (string, ' ');
			g_string_append (string, item->description);
			g_string_append_c (string, '\n');
		} else {
			g_string_append_c (string, '\n');
			for (j = 0; j < max_len + 1; j++)
				g_string_append_c (string, ' ');
			g_string_append (string, item->description);
			g_string_append_c (string, '\n');
		}
	}

	/* remove trailing newline */
	if (string->len > 0)
		g_string_set_size (string, string->len - 1);

	return g_string_free (string, FALSE);
}

/**
 * fu_util_run:
 **/
static gboolean
fu_util_run (FuUtilPrivate *priv, const gchar *command, gchar **values, GError **error)
{
	guint i;
	FuUtilItem *item;
	_cleanup_string_free_ GString *string = NULL;

	/* find command */
	for (i = 0; i < priv->cmd_array->len; i++) {
		item = g_ptr_array_index (priv->cmd_array, i);
		if (g_strcmp0 (item->name, command) == 0)
			return item->callback (priv, values, error);
	}

	/* not found */
	string = g_string_new ("");
	/* TRANSLATORS: error message */
	g_string_append_printf (string, "%s\n",
				_("Command not found, valid commands are:"));
	for (i = 0; i < priv->cmd_array->len; i++) {
		item = g_ptr_array_index (priv->cmd_array, i);
		g_string_append_printf (string, " * %s %s\n",
					item->name,
					item->arguments ? item->arguments : "");
	}
	g_set_error_literal (error, FU_ERROR, FU_ERROR_NO_SUCH_CMD, string->str);
	return FALSE;
}

/**
 * fu_util_get_devices:
 **/
static gboolean
fu_util_get_devices (FuUtilPrivate *priv, gchar **values, GError **error)
{
	_cleanup_object_unref_ GDBusConnection *conn = NULL;
	_cleanup_object_unref_ GDBusProxy *proxy = NULL;
	_cleanup_variant_unref_ GVariant *val = NULL;
	const gchar **guids = NULL;
	guint i;

	conn = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, error);
	if (conn == NULL)
		return FALSE;
	proxy = g_dbus_proxy_new_sync (conn,
				       G_DBUS_PROXY_FLAGS_NONE,
				       NULL,
				       FWUPD_DBUS_SERVICE,
				       FWUPD_DBUS_PATH,
				       FWUPD_DBUS_INTERFACE,
				       NULL,
				       error);
	if (proxy == NULL)
		return FALSE;
	val = g_dbus_proxy_call_sync (proxy,
				      "GetDevices",
				      NULL,
				      G_DBUS_CALL_FLAGS_NONE,
				      -1,
				      NULL,
				      error);
	if (val == NULL)
		return FALSE;
	g_variant_get (val, "(^a&s)", &guids);
	g_assert (guids != NULL);
	if (guids[0] == NULL)
		g_print ("No hardware detected with firmware update capaility\n");
	for (i = 0; guids[i] != NULL; i++)
		g_print ("%i: %s\n", i, guids[i]);
	return TRUE;
}

/**
 * fu_util_update_offline:
 **/
static gboolean
fu_util_update_offline (FuUtilPrivate *priv, gchar **values, GError **error)
{
	//FIXME
	return TRUE;
}

/**
 * fu_util_ignore_cb:
 **/
static void
fu_util_ignore_cb (const gchar *log_domain, GLogLevelFlags log_level,
		   const gchar *message, gpointer user_data)
{
}

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	FuUtilPrivate *priv;
	gboolean ret;
	gboolean verbose = FALSE;
	guint retval = 1;
	_cleanup_error_free_ GError *error = NULL;
	_cleanup_free_ gchar *cmd_descriptions = NULL;
	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
			/* TRANSLATORS: command line option */
			_("Show extra debugging information"), NULL },
		{ NULL}
	};

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* create helper object */
	priv = g_new0 (FuUtilPrivate, 1);

	/* add commands */
	priv->cmd_array = g_ptr_array_new_with_free_func ((GDestroyNotify) fu_util_item_free);
	fu_util_add (priv->cmd_array,
		     "get-devices",
		     NULL,
		     /* TRANSLATORS: command description */
		     _("Get all devices that support firmware updates"),
		     fu_util_get_devices);
	fu_util_add (priv->cmd_array,
		     "update-offline",
		     NULL,
		     /* TRANSLATORS: command description */
		     _("Install the update the next time the computer is rebooted"),
		     fu_util_update_offline);

	/* sort by command name */
	g_ptr_array_sort (priv->cmd_array,
			  (GCompareFunc) fu_sort_command_name_cb);

	/* get a list of the commands */
	priv->context = g_option_context_new (NULL);
	cmd_descriptions = fu_util_get_descriptions (priv->cmd_array);
	g_option_context_set_summary (priv->context, cmd_descriptions);

	/* TRANSLATORS: program name */
	g_set_application_name (_("Firmware Update"));
	g_option_context_add_main_entries (priv->context, options, NULL);
	ret = g_option_context_parse (priv->context, &argc, &argv, &error);
	if (!ret) {
		/* TRANSLATORS: the user didn't read the man page */
		g_print ("%s: %s\n", _("Failed to parse arguments"),
			 error->message);
		goto out;
	}

	/* set verbose? */
	if (verbose) {
		g_setenv ("FWUPD_VERBOSE", "1", FALSE);
	} else {
		g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				   fu_util_ignore_cb, NULL);
	}

	/* run the specified command */
	ret = fu_util_run (priv, argv[1], (gchar**) &argv[2], &error);
	if (!ret) {
		if (g_error_matches (error, FU_ERROR, FU_ERROR_NO_SUCH_CMD)) {
			_cleanup_free_ gchar *tmp = NULL;
			tmp = g_option_context_get_help (priv->context, TRUE, NULL);
			g_print ("%s", tmp);
		} else {
			g_print ("%s\n", error->message);
		}
		goto out;
	}

	/* success */
	retval = 0;
out:
	if (priv != NULL) {
		if (priv->cmd_array != NULL)
			g_ptr_array_unref (priv->cmd_array);
		g_option_context_free (priv->context);
		g_free (priv);
	}
	return retval;
}

