/*
 * Copyright (C) 2015 Kate Hsuan <hpa@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define G_LOG_DOMAIN "FuMain"

#include "config.h"

#include <fwupdplugin.h>

#include <fcntl.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#ifdef HAVE_GIO_UNIX
#include <gio/gunixfdlist.h>
#include <glib-unix.h>
#endif
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>

#include "fu-console.h"
#include "fu-plugin-private.h"
#include "fu-util-common.h"
#include "fu-util-repair.h"


static gboolean 
grubby_set_lockdown (gboolean enable, GError **error)
{
	g_autofree gchar *output = NULL;
	const gchar *argv_grubby[] = {"",
				      "--update-kernel=DEFAULT",
				      "--args=lockdown=confidentiality",
				      NULL};
	g_autofree gchar *grubby = NULL;
	grubby = fu_path_find_program("grubby", NULL);
	argv_grubby[0] = grubby;

	if (enable)
		argv_grubby[2] = "--args=lockdown=confidentiality";
	else
		argv_grubby[2] = "--remove-args=lockdown=confidentiality";

	if (!g_spawn_sync(NULL,
			  (gchar **)argv_grubby,
			  NULL,
			  G_SPAWN_DEFAULT,
			  NULL,
			  NULL,
			  &output,
			  NULL,
			  NULL,
			  error))
		return FALSE;

	return TRUE;
}

static gboolean
set_secure_boot (gboolean enable, GError **error)
{

	return TRUE;
}

static gboolean
fu_util_repair_kernel_lockdown_cb (GError **error)
{
	/* set lockdown to kernel parameter */
	/* grubby --update-kernel=DEFAULT --args="lockdown=confidentiality" */
	g_printf("Fix lockdown\n");
	return grubby_set_lockdown (TRUE, NULL);
}

static gboolean
fu_util_repair_secure_boot_cb (GError **error)
{
	g_printf("Fix secure boot\n");
	set_secure_boot (TRUE, error);
	return TRUE;
}

static gboolean
fu_util_repair_undo_kernel_lockdown_cb (GError **error)
{
	return grubby_set_lockdown (FALSE, NULL);
}

static gboolean
fu_util_undo_repair_secure_boot_cb (GError **error)
{
	g_printf("undo secure boot\n");
	set_secure_boot (FALSE, error);
	return TRUE;
}

GList *
fu_util_repair_init(void)
{
	g_autoptr(GList) fu_repair_list = NULL;
	struct FuUtilRepairPrivate *repair;

	for (gint i = 0; i < FU_UTIL_REPAIR_LAST; i++) {
		repair = g_malloc0(sizeof(struct FuUtilRepairPrivate));
		switch (i) {
		case FU_UTIL_REPAIR_KERNEL_LOCKDOWN:
			repair->repair_id = FU_UTIL_REPAIR_KERNEL_LOCKDOWN;
			repair->name = g_string_new("kernel-lockdown");
			repair->do_callback = fu_util_repair_kernel_lockdown_cb;
			repair->undo_callback = fu_util_repair_undo_kernel_lockdown_cb;
			break;
		case FU_UTIL_REPAIR_SECURE_BOOT:
			repair->repair_id = FU_UTIL_REPAIR_SECURE_BOOT;
			repair->name = g_string_new("secure-boot");
			repair->do_callback = fu_util_repair_secure_boot_cb;
			repair->undo_callback = fu_util_undo_repair_secure_boot_cb;
			break;
		default:
			return fu_repair_list;
		};
		fu_repair_list = g_list_append(fu_repair_list, repair);
	}

	return g_steal_pointer (&fu_repair_list);
}

void
fu_util_repair_list(FuConsole *console, GList *fu_repair_list)
{
	guint i;
	gpointer *tmp;
	g_autoptr(GString) repair_msg;
	struct FuUtilRepairPrivate *repair;

	repair_msg = g_string_new(NULL);
	for (i = 0; i < g_list_length(fu_repair_list) ; i++)
	{
		tmp = g_list_nth_data (fu_repair_list, i);
		if (tmp) {
			repair = (struct FuUtilRepairPrivate*)tmp;
			g_string_append_printf (repair_msg, "%s\n", repair->name->str);
		}
	}
	fu_console_print_full(console, FU_CONSOLE_PRINT_FLAG_NONE, "%s\n", repair_msg->str);
}

static guint
fu_util_repair_name_to_id (GList *fu_repair_list, gchar *name)
{
	struct FuUtilRepairPrivate *repair;

	for (guint i = 0; i < g_list_length(fu_repair_list) ; i++)
	{
		repair = g_list_nth_data (fu_repair_list, i);
		if (repair) {
			if (!g_strcmp0 (name, repair->name->str))
				return repair->repair_id;
		}
	}

	return FU_UTIL_REPAIR_LAST;
}

gboolean
fu_util_repair_do_undo (GList *fu_repair_list, gchar *repair_name, gboolean is_do, GError **error)
{
	guint repair_id;
	struct FuUtilRepairPrivate *repair;

	repair_id = fu_util_repair_name_to_id (fu_repair_list, repair_name);
	repair = g_list_nth_data (fu_repair_list, repair_id);
	
	if (repair) {
		if (is_do)
			repair->do_callback (error);
		else if (!is_do)
			repair->undo_callback (error);
	} else {
		g_set_error_literal (error,
				     FWUPD_ERROR_NOT_SUPPORTED,
				     FWUPD_ERROR_NOTHING_TO_DO,
				     /*TRANSLATOR: This is an error string */
				     _("Repair item is not found.")
				     );
		return FALSE;
	}

	return TRUE;
}






