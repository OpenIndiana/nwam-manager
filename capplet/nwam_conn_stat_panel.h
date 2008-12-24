/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:set expandtab ts=4 shiftwidth=4: */
/* 
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * CDDL HEADER START
 * 
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 * 
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * 
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 * 
 * CDDL HEADER END
 * 
 * File:   nwam_conn_status_panel.h
 *
 * Created on May 9, 2007, 11:13 AM
 * 
 */

#ifndef _NWAM_CONN_STATUS_PANEL_H
#define	_NWAM_CONN_STATUS_PANEL_H

G_BEGIN_DECLS


#define NWAM_TYPE_CONN_STATUS_PANEL               (nwam_conn_status_panel_get_type ())
#define NWAM_CONN_STATUS_PANEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), NWAM_TYPE_CONN_STATUS_PANEL, NwamConnStatusPanel))
#define NWAM_CONN_STATUS_PANEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), NWAM_TYPE_CONN_STATUS_PANEL, NwamConnStatusPanelClass))
#define NWAM_IS_CONN_STATUS_PANEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NWAM_TYPE_CONN_STATUS_PANEL))
#define NWAM_IS_CONN_STATUS_PANEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), NWAM_TYPE_CONN_STATUS_PANEL))
#define NWAM_CONN_STATUS_PANEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), NWAM_TYPE_CONN_STATUS_PANEL, NwamConnStatusPanelClass))


typedef struct _NwamConnStatusPanel		NwamConnStatusPanel;
typedef struct _NwamConnStatusPanelClass	NwamConnStatusPanelClass;
typedef struct _NwamConnStatusPanelPrivate	NwamConnStatusPanelPrivate;

struct _NwamConnStatusPanel
{
	GObject                      object;

	/*< private >*/
	NwamConnStatusPanelPrivate    *prv;
};

struct _NwamConnStatusPanelClass
{
	GObjectClass                parent_class;
};

GType nwam_conn_status_panel_get_type (void) G_GNUC_CONST;

NwamConnStatusPanel* nwam_conn_status_panel_new(NwamCappletDialog *pref_dialog);

extern void nwam_conn_status_panel_set_ncp(NwamConnStatusPanel *self, NwamuiNcp* ncp );

void nwam_conn_status_add (NwamConnStatusPanel *self, NwamuiNcu* connection);

G_END_DECLS

#endif	/* _NWAM_CONN_STATUS_PANEL_H */
