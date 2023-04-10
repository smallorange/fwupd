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

#include "fwupd-common-private.h"
#include "fwupd-device-private.h"
#include "fwupd-plugin-private.h"
#include "fwupd-release-private.h"
#include "fwupd-remote-private.h"

#include "fu-console.h"
#include "fu-plugin-private.h"
#include "fu-polkit-agent.h"
#include "fu-util-bios-setting.h"
#include "fu-util-common.h"

#ifdef HAVE_SYSTEMD
#include "fu-systemd.h"
#endif

#include "fu-util-repair.h"



static gboolean fu_util_repair_kernel_lockdown_cb (void)
{
	g_printf("Fix kernel locK\n");

	return TRUE;
}

static gboolean fu_util_repair_secure_boot_cb (void)
{
	g_printf("Fix secure boot\n");

	return TRUE;
}


static gboolean fu_util_repair_undo_kernel_lockdown_cb (void)
{
	g_printf("undo kernel lockdown\n");

	return TRUE;
}

static gboolean fu_util_undo_repair_secure_boot_cb (void)
{
	g_printf("undo secure boot\n");

	return TRUE;
}


GList *fu_util_repair_init(void)
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

void fu_util_repair_list(FuConsole *console, GList *fu_repair_list)
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

static guint fu_util_repair_name_to_id (GList *fu_repair_list, gchar *name)
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

gboolean fu_util_repair_do_undo (GList *fu_repair_list, gchar *repair_name, gboolean is_do, GError **error)
{
	guint repair_id;
	struct FuUtilRepairPrivate *repair;

	repair_id = fu_util_repair_name_to_id (fu_repair_list, repair_name);
	repair = g_list_nth_data (fu_repair_list, repair_id);
	
	if (repair) {
		if (is_do)
			repair->do_callback ();
		else if (!is_do)
			repair->undo_callback ();
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






