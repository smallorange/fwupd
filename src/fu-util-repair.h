/*
 * Copyright (C) 2017 Kate Hsuan <hpa@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <fwupdplugin.h>

#include <json-glib/json-glib.h>

#include "fwupd-bios-setting-private.h"
#include "fwupd-security-attr-private.h"

#include "fu-console.h"

typedef gboolean (*FuUtilRepairCmdFunc)(void);
struct FuUtilRepairPrivate{
	guint repair_id;
	GString *name;
	FuUtilRepairCmdFunc do_callback;
	FuUtilRepairCmdFunc undo_callback;
};

typedef enum {
	FU_UTIL_REPAIR_KERNEL_LOCKDOWN,
	FU_UTIL_REPAIR_SECURE_BOOT,
	FU_UTIL_REPAIR_LAST
} FuRepairId;

GList *fu_util_repair_init(void);
void fu_util_repair_list(FuConsole *console, GList *fu_repair_list);
gboolean fu_util_repair_do_undo (GList *fu_repair_list, gchar *repair_name, gboolean is_do, GError **error);
