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

#ifndef __FU_PROVIDER_UEFI_H
#define __FU_PROVIDER_UEFI_H

#include <glib-object.h>

#include "fu-device.h"
#include "fu-provider-uefi.h"

G_BEGIN_DECLS

#define FU_TYPE_PROVIDER_UEFI		(fu_provider_uefi_get_type ())
#define FU_PROVIDER_UEFI(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), FU_TYPE_PROVIDER_UEFI, FuProviderUefi))
#define FU_PROVIDER_UEFI_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), FU_TYPE_PROVIDER_UEFI, FuProviderUefiClass))
#define FU_IS_PROVIDER_UEFI(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), FU_TYPE_PROVIDER_UEFI))
#define FU_IS_PROVIDER_UEFI_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), FU_TYPE_PROVIDER_UEFI))
#define FU_PROVIDER_UEFI_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), FU_TYPE_PROVIDER_UEFI, FuProviderUefiClass))

typedef struct _FuProviderUefiPrivate	FuProviderUefiPrivate;
typedef struct _FuProviderUefi		FuProviderUefi;
typedef struct _FuProviderUefiClass	FuProviderUefiClass;

struct _FuProviderUefi
{
	 GObject			 parent;
	 FuProviderUefiPrivate		*priv;
};

struct _FuProviderUefiClass
{
	GObjectClass	parent_class;
	void		(* device_added)	(FuProviderUefi	*provider_uefi,
						 FuDevice	*device);
	void		(* device_removed)	(FuProviderUefi	*provider_uefi,
						 FuDevice	*device);
};

GType		 fu_provider_uefi_get_type	(void);
FuProviderUefi	*fu_provider_uefi_new		(void);
void		 fu_provider_uefi_coldplug	(FuProviderUefi	*provider_uefi);

G_END_DECLS

#endif /* __FU_PROVIDER_UEFI_H */

