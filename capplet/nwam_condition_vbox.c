/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:set expandtab ts=4 shiftwidth=4: */
/* 
 * Copyright 2007-2009 Sun Microsystems, Inc.  All rights reserved.
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
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <strings.h>
#include <stdlib.h>

#include "libnwamui.h"
#include "nwam_pref_iface.h"
#include "nwam_condition_vbox.h"
#include "capplet-utils.h"

#define	_CU_COND_FILTER_VALUE	"custom_cond_combo_filter_value"
#define TABLE_PREF_PARENT "table_pref_parent"
#define TABLE_ROW_COMBO1 "condition_vbox_row_combo1"
#define TABLE_ROW_COMBO2 "condition_vbox_row_combo2"
#define TABLE_ROW_NOTEBOOK "condition_vbox_row_notebook"
#define TABLE_ROW_ENTRY "condition_vbox_row_entry"
#define TABLE_ROW_NCP_COMBO "condition_vbox_row_ncp_combo"
#define TABLE_ROW_NCU_COMBO "condition_vbox_row_ncu_combo"
#define TABLE_ROW_ENM_COMBO "condition_vbox_row_enm_combo"
#define TABLE_ROW_LOC_COMBO "condition_vbox_row_loc_combo"
#define TABLE_ROW_WIFI_COMBO_ENTRY "condition_vbox_row_wifi_combo_entry"
#define TABLE_ROW_ADD "condition_vbox_row_add"
#define TABLE_ROW_REMOVE "condition_vbox_row_remove"
#define TABLE_ROW_CDATA "condition_vbox_row_cdata"
#define TABLE_CONDITION_SHOW_NO_COND "table_condition_show_no_cond"

#define INVALID_IP_MARKER             "<INVALID_IP_MARKER>"

enum {
    S_CONDITION_ADD,
    S_CONDITION_REMOVE,
    LAST_SIGNAL
};

enum {
    VALUE_ENTRY_PAGE = 0,
    VALUE_NCP_COMBO_PAGE,
    VALUE_NCU_COMBO_PAGE,
    VALUE_ENM_COMBO_PAGE,
    VALUE_LOC_COMBO_PAGE,
    VALUE_WIFI_COMBO_ENTRY_PAGE
};

static guint cond_signals[LAST_SIGNAL] = {0};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	NWAM_TYPE_CONDITION_VBOX, NwamConditionVBoxPrivate)) 

struct _NwamConditionVBoxPrivate {
    /* Widget Pointers */
    GList    *table_box_cache;
    guint     table_line_num;
    gboolean  hide_location;

    NwamuiObject *selected_object;
};

static void nwam_pref_init (gpointer g_iface, gpointer iface_data);
static gboolean refresh(NwamPrefIFace *iface, gpointer user_data, gboolean force);
static gboolean apply(NwamPrefIFace *iface, gpointer user_data);
static gboolean cancel(NwamPrefIFace *iface, gpointer user_data);

static void nwam_condition_vbox_finalize (NwamConditionVBox *self);
static void table_lines_init (NwamConditionVBox *self, NwamuiObject *object);
static void table_lines_free (NwamConditionVBox *self);
static GtkWidget *table_condition_new (NwamConditionVBox *self);
static GtkWidget* table_condition_insert (GtkWidget *clist, guint row, NwamuiCond* cond);
static void table_condition_delete(GtkWidget *clist, GtkWidget *crow);

static void update_condition_row_widgets( GtkWidget* widget, gpointer user_data );
static void table_add_condition_cb(GtkButton *button, gpointer  data);
static void table_delete_condition_cb(GtkButton *button, gpointer  data);
static void table_line_cache_all_cb (GtkWidget *widget, gpointer data);

extern GtkTreeModel *table_get_condition_subject_model ();
extern GtkTreeModel *table_get_condition_predicate_model ();
extern GtkTreeModel *table_get_condition_ncp_list_model ();
extern GtkTreeModel *table_get_condition_ncu_list_model ();
extern GtkTreeModel *table_get_condition_enm_list_model ();
extern GtkTreeModel *table_get_condition_loc_list_model ();
extern GtkTreeModel *table_get_condition_wifi_list_model ();

static void condition_field_op_changed_cb( GtkWidget* widget, gpointer data );
static void condition_value_changed_cb( GtkWidget* widget, gpointer data );
static void condition_gobject_combo_changed_cb( GtkComboBox* combo, gpointer data );
static void condition_wifi_combo_changed_cb( GtkComboBox* combo, gpointer data );

static void default_condition_add_signal_handler (NwamConditionVBox *self, GObject* data, gpointer user_data);
static void default_condition_remove_signal_handler (NwamConditionVBox *self, GObject* data, gpointer user_data);


G_DEFINE_TYPE_EXTENDED (NwamConditionVBox,
    nwam_condition_vbox,
    GTK_TYPE_VBOX,
    0,
    G_IMPLEMENT_INTERFACE (NWAM_TYPE_PREF_IFACE, nwam_pref_init))

static gboolean
refresh(NwamPrefIFace *iface, gpointer user_data, gboolean force)
{
	NwamConditionVBox *self = NWAM_CONDITION_VBOX(iface);
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(iface);

	if (prv->selected_object != user_data) {
		if (prv->selected_object) {
			g_object_unref(prv->selected_object);
		}
		if ((prv->selected_object = user_data) != NULL) {
			g_object_ref(prv->selected_object);
		}
		force = TRUE;
	}

	if (force) {
		if (prv->selected_object) {
			table_lines_init (self, NWAMUI_OBJECT(user_data));
		}
	}

	return TRUE;
}

static void
set_combobox_unfiltered_index( GtkComboBox* combo, gint index ) 
{
    GtkTreeModel       *model = NULL;

    model = GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));
    if ( GTK_IS_TREE_MODEL_FILTER( model ) ) { /* need to to select based on filter */
        GtkTreeModel  *unfiltered = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER(model) );
        GtkTreeIter    unfiltered_iter;
        GtkTreeIter    filtered_iter;

        if ( gtk_tree_model_iter_nth_child( unfiltered, &unfiltered_iter, NULL, index ) ) {
            if ( gtk_tree_model_filter_convert_child_iter_to_iter( GTK_TREE_MODEL_FILTER(model), 
                                                                   &filtered_iter, &unfiltered_iter ) ) {
                gtk_combo_box_set_active_iter (GTK_COMBO_BOX(combo), &filtered_iter );
            }
        }
    }
    else {
        gtk_combo_box_set_active (GTK_COMBO_BOX(combo), index);
    }
}

static gint
get_unfiltered_index_from_combo( GtkComboBox* combo ) 
{
    GtkTreeIter         iter, realiter;
    GtkTreeModel       *filter;
    GtkTreeModel       *model;

    gint                index = 0;

    filter = GTK_TREE_MODEL(gtk_combo_box_get_model (GTK_COMBO_BOX(combo)));
    if ( filter != NULL && GTK_IS_TREE_MODEL_FILTER(filter) ) {
        model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER(filter));
    }
    else { /* There is no filter */
        model = filter;
        filter = NULL;
    }

    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX(combo), &iter)) {
        if ( filter != NULL ) {
            gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER(filter),
                                                              &realiter, &iter);
            gtk_tree_model_get (model, &realiter, 0, &index, -1);
        }
        else {
            gtk_tree_model_get (model, &iter, 0, &index, -1);
        }
    }

    return (index);
}
/* Returns an error string if failed, NULL on success */
static gchar*
update_cond_from_row( GtkWidget *widget, NwamuiCond *cond )
{
	GtkBox             *box = NULL;
	GtkComboBox        *combo, *combo1, *combo2, *ncp_combo, *ncu_combo, *enm_combo, *loc_combo;
    GtkComboBoxEntry   *wifi_combo_entry;
    GtkNotebook        *value_nb;
	GtkEntry           *entry;
	GtkButton          *add, *remove;
    GtkTreeIter         iter;
    GtkTreeModel       *model = NULL;
    nwamui_cond_field_t field;
    nwamui_cond_op_t    oper;
    gchar*              value = NULL;
	
    box = GTK_BOX(widget);
    combo1 = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_COMBO1));
    combo2 = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_COMBO2));
    value_nb = GTK_NOTEBOOK(g_object_get_data(G_OBJECT(box), TABLE_ROW_NOTEBOOK ));
    entry = GTK_ENTRY(g_object_get_data(G_OBJECT(box), TABLE_ROW_ENTRY));
    ncp_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_NCP_COMBO));
    ncu_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_NCU_COMBO));
    enm_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_ENM_COMBO));
    loc_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_LOC_COMBO));
    wifi_combo_entry = GTK_COMBO_BOX_ENTRY(g_object_get_data(G_OBJECT(box), TABLE_ROW_WIFI_COMBO_ENTRY));

    /* Get the Field */
    field = (nwamui_cond_field_t)get_unfiltered_index_from_combo( combo1 );

    if ( field == NWAMUI_COND_FIELD_LAST ) {
        nwamui_cond_set_field( cond, (nwamui_cond_field_t)field);
        return( NULL );
    }

    /* Get the Operator */
    oper = (nwamui_cond_op_t)get_unfiltered_index_from_combo( combo2 );

    /* Get the appropriate value, depending on field */
    if (field == NWAMUI_COND_FIELD_NCP) {
        combo = ncp_combo;
        model = gtk_combo_box_get_model(ncp_combo);
    }
    else if (field == NWAMUI_COND_FIELD_NCU) {
        combo = ncu_combo;
        model = gtk_combo_box_get_model(ncu_combo);
    }
    else if (field == NWAMUI_COND_FIELD_LOC) {
        combo = loc_combo;
        model = gtk_combo_box_get_model(loc_combo);
    }
    else if (field == NWAMUI_COND_FIELD_ENM ) {
        combo = enm_combo;
        model = gtk_combo_box_get_model(enm_combo);
    }
    
    
    if (model != NULL) {
        if ( gtk_combo_box_get_active_iter( combo, &iter ) ) {
            gpointer    ptr;

            gtk_tree_model_get (model, &iter, 0, &ptr, -1);

            if ( gtk_tree_model_get_column_type(model, 0) == G_TYPE_OBJECT 
                 || gtk_tree_model_get_column_type(model, 0) == NWAMUI_TYPE_NCP
                 || gtk_tree_model_get_column_type(model, 0) == NWAMUI_TYPE_NCU
                 || gtk_tree_model_get_column_type(model, 0) == NWAMUI_TYPE_ENM
                 || gtk_tree_model_get_column_type(model, 0) == NWAMUI_TYPE_ENV ) {
                GObject* obj = (GObject*)ptr;

                if ( NWAMUI_IS_NCU( obj ) ) {
                    /* NCU objects need to be qualified as link or interface,
                     * etc. 
                     */
                    value = nwamui_ncu_get_nwam_qualified_name ( NWAMUI_NCU(obj) );
                }
                else if ( NWAMUI_IS_NCP(obj) || NWAMUI_IS_ENM(obj) || NWAMUI_IS_ENV(obj)) {
                    value = g_strdup(nwamui_object_get_name(NWAMUI_OBJECT(obj)));
                }
            }

        }
        else {
            /* Nothing selected - shouldn't happen but could if there was
             * nothing to select.
             */
            return( g_strdup_printf(_("Invalid value specified for field :\n\n    %s"), 
                                    nwamui_cond_field_to_str( field ))  );
        }
    }
    else {
        if (field == NWAMUI_COND_FIELD_ESSID ) {
            value = g_strdup(gtk_combo_box_get_active_text( GTK_COMBO_BOX(wifi_combo_entry) ));
        }
        else {
            value = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

            if (field == NWAMUI_COND_FIELD_IP_ADDRESS) {
                nwamui_entry_validation_flags_t flags;

                flags = NWAMUI_ENTRY_VALIDATION_IS_V4|
                        NWAMUI_ENTRY_VALIDATION_IS_V6;

                switch ( oper ) {
                case NWAMUI_COND_OP_IS_IN_RANGE:
                case NWAMUI_COND_OP_IS_NOT_IN_RANGE:
                    flags |= NWAMUI_ENTRY_VALIDATION_ALLOW_PREFIX;
                default:
                    break;
                }
                if ( !nwamui_util_validate_text_entry( GTK_WIDGET(entry), value, flags, TRUE, TRUE ) ) {
                    return( INVALID_IP_MARKER ); /* user marker since validation fn will show dialog */
                }
            }
            else if ( field == NWAMUI_COND_FIELD_BSSID) {
                if ( ! nwamui_util_validate_text_entry( GTK_WIDGET(entry), value, 
                                                        NWAMUI_ENTRY_VALIDATION_IS_ETHERS,
                                                        TRUE, TRUE ) ) {
                    return( INVALID_IP_MARKER ); /* user marker since validation fn will show dialog */
                }
            }
        }
        if ( value == NULL || strlen(value) == 0 ) {
            return( g_strdup_printf(_("Invalid empty value specified for field :\n\n    %s"), 
                                    nwamui_cond_field_to_str( field ))  );
        }
    }

    nwamui_cond_set_field( cond, (nwamui_cond_field_t)field);
    nwamui_cond_set_oper( cond, (nwamui_cond_op_t)oper);
    nwamui_cond_set_value( cond, value);

    g_free(value);

    return( NULL );
}

typedef struct {
    gboolean    validation_error;
    gchar      *error;
    GList      *cond_list;
}
apply_data_t;

static void
apply_table_row( GtkWidget* widget, gpointer user_data )
{
    NwamuiCond     *cond = NWAMUI_COND(g_object_get_data(G_OBJECT(widget), TABLE_ROW_CDATA));
    apply_data_t   *_apply = (apply_data_t*)user_data;
    gchar          *err;

    if ( _apply->validation_error ) {
        /* Skip if validation error has occurred */
        return;
    }

    if ( cond == NULL ) {
        g_warning("Got NULL condition from table row");
        _apply->error = g_strdup(_("Got empty condition"));
        _apply->validation_error = TRUE;
        return;
    }

    if ( (err = update_cond_from_row( widget, cond )) != NULL ) {
        /* Error occurred validating */
        if ( g_strcmp0( err, INVALID_IP_MARKER ) == 0 ) {
            _apply->error = NULL; /* Dialog should have already been shown */
        }
        else {
            _apply->error = err;
        }
        _apply->validation_error = TRUE;
        return;
    }


    if ( nwamui_cond_get_field(cond) != NWAMUI_COND_FIELD_LAST ) {
        char *str = nwamui_cond_to_string( cond );
        g_debug("Appending condition : %s", str?str:"<NULL>" );
        free(str);

        _apply->cond_list = g_list_append( _apply->cond_list, (gpointer)g_object_ref(cond) );
    }
}

static gboolean
apply(NwamPrefIFace *iface, gpointer user_data)
{
	NwamConditionVBoxPrivate   *prv = GET_PRIVATE(iface);
    GList                      *conditions_list = NULL;
    apply_data_t                apply_data = { FALSE, NULL, NULL };
    
    gtk_container_foreach( GTK_CONTAINER(iface), apply_table_row, &apply_data );

    if ( apply_data.validation_error ) {
        /* Only show error if string is set, it's possible that it may not be
         * if something already has shown a dialog.
         */
        if ( apply_data.error != NULL ) {
            GtkWindow              *top_level = NULL;
            top_level = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(iface)));

            nwamui_util_show_message(GTK_WINDOW(top_level), 
                                     GTK_MESSAGE_ERROR, _("Validation Error"), apply_data.error, TRUE );

            g_free( apply_data.error );
        }

        nwamui_util_free_obj_list( apply_data.cond_list );
        return( FALSE );
    }

	nwamui_object_set_conditions(NWAMUI_OBJECT(prv->selected_object), apply_data.cond_list );

    return( TRUE );
}

static gboolean
cancel(NwamPrefIFace *iface, gpointer user_data)
{
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(iface);
	NwamConditionVBox *self = NWAM_CONDITION_VBOX(iface);

/* 	nwamui_object_get_conditions(NWAMUI_OBJECT(prv->selected_object)); */
	nwam_pref_refresh(NWAM_PREF_IFACE(self), user_data, TRUE);
}

static void
nwam_pref_init (gpointer g_iface, gpointer iface_data)
{
	NwamPrefInterface *iface = (NwamPrefInterface *)g_iface;
	iface->refresh = refresh;
	iface->apply = apply;
	iface->cancel = cancel;
	iface->help = NULL;
	iface->dialog_run = NULL;
}

static void
nwam_condition_vbox_class_init (NwamConditionVBoxClass *klass)
{
    /* Pointer to GObject Part of Class */
    GObjectClass *gobject_class = (GObjectClass*) klass;
        
    /* Override Some Function Pointers */
    gobject_class->finalize = (void (*)(GObject*)) nwam_condition_vbox_finalize;

    g_type_class_add_private (klass, sizeof (NwamConditionVBoxPrivate));

    /* Create some signals */
    cond_signals[S_CONDITION_ADD] =   
            g_signal_new ("condition_add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (NwamConditionVBoxClass, condition_add),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER, 
                  G_TYPE_NONE,                  /* Return Type */
                  1,                            /* Number of Args */
                  G_TYPE_POINTER);               /* Types of Args */
    
    cond_signals[S_CONDITION_REMOVE] =   
            g_signal_new ("condition_remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (NwamConditionVBoxClass, condition_remove),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER, 
                  G_TYPE_NONE,                  /* Return Type */
                  1,                            /* Number of Args */
                  G_TYPE_POINTER);               /* Types of Args */

    klass->condition_add = default_condition_add_signal_handler;
    klass->condition_remove = default_condition_remove_signal_handler;
}
        
static void
nwam_condition_vbox_init (NwamConditionVBox *self)
{
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(self);
}

extern guint
nwam_condition_vbox_get_num_lines (NwamConditionVBox *self)
{
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(self);

    return( prv->table_line_num );
}

static void
nwam_condition_vbox_finalize (NwamConditionVBox *self)
{
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(self);

	table_lines_free (self);

	if (prv->selected_object) {
		g_object_unref(prv->selected_object);
	}

	G_OBJECT_CLASS(nwam_condition_vbox_parent_class)->finalize(G_OBJECT(self));
}

static void
_cu_cond_combo_filter_value (GtkWidget *combo, gint value)
{
    g_object_set_data (G_OBJECT(combo),
                       _CU_COND_FILTER_VALUE,
                       (gpointer)value);

    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER(gtk_combo_box_get_model(GTK_COMBO_BOX(combo))));
}

static gboolean
_cu_cond_combo_filter_visible_cb (GtkTreeModel *model,
  GtkTreeIter *iter,
  gpointer data)
{
    GtkWidget *combo = GTK_WIDGET(data);
    GtkWidget *hbox = gtk_widget_get_parent(GTK_WIDGET(data));
    GtkWidget *vbox = gtk_widget_get_parent(hbox);
	NwamConditionVBoxPrivate *prv;
    gint row_index;
    gint value;
    gint row_data;

    if (vbox) {
        prv = GET_PRIVATE(vbox);
    } else {
        return FALSE;
    }

    value = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(combo),
                                              _CU_COND_FILTER_VALUE));

    gtk_tree_model_get (model, iter, 0, &row_data, -1);

/*     gtk_container_child_get(GTK_CONTAINER(vbox), */
/*       hbox, */
/*       "position", &row_index, */
/*       NULL); */

    if (model == (gpointer)table_get_condition_subject_model ()) {
        /* Filter out Location as possible subject if configuring a Location */
        if (prv->hide_location && row_data == NWAMUI_COND_FIELD_LOC) {
            return( FALSE );
        }
        /* Only show now <no condition> if there is only one condition row. */
        if (prv->table_line_num == 1 && row_data == NWAMUI_COND_FIELD_LAST) {
            return( TRUE );
        }
        else {
            return row_data != NWAMUI_COND_FIELD_LAST;
        }
    } else {
        switch (row_data) {
        case NWAMUI_COND_OP_IS:
        case NWAMUI_COND_OP_IS_NOT:
            return (value != NWAMUI_COND_FIELD_NCU) &&
              (value != NWAMUI_COND_FIELD_ENM);
        case NWAMUI_COND_OP_INCLUDE:
        case NWAMUI_COND_OP_DOES_NOT_INCLUDE:
            return (value == NWAMUI_COND_FIELD_NCU) ||
              (value == NWAMUI_COND_FIELD_ENM);
        case NWAMUI_COND_OP_IS_IN_RANGE:
        case NWAMUI_COND_OP_IS_NOT_IN_RANGE:
            return value == NWAMUI_COND_FIELD_IP_ADDRESS;
        case NWAMUI_COND_OP_CONTAINS:
        case NWAMUI_COND_OP_DOES_NOT_CONTAIN:
            return (value == NWAMUI_COND_FIELD_ADV_DOMAIN) ||
              (value == NWAMUI_COND_FIELD_SYS_DOMAIN) ||
              (value == NWAMUI_COND_FIELD_ESSID);
        default:
            return FALSE;
        }
    }
}

static void
_cu_cond_combo_cell_cb (GtkCellLayout *cell_layout,
  GtkCellRenderer   *renderer,
  GtkTreeModel      *model,
  GtkTreeIter       *iter,
  gpointer           data)
{
    GtkTreeModel *realmodel;
    GtkTreeModelFilter *filter;
    GtkTreeIter realiter;
    const gchar *str;

    gint row_data = 0;

    filter = GTK_TREE_MODEL_FILTER(model);
    realmodel = gtk_tree_model_filter_get_model (filter);
    gtk_tree_model_filter_convert_iter_to_child_iter (filter,
      &realiter, iter);
	gtk_tree_model_get(realmodel, &realiter, 0, &row_data, -1);

	if (realmodel == (gpointer)table_get_condition_subject_model ()) {
        if (row_data != NWAMUI_COND_FIELD_LAST) {
            str = nwamui_cond_field_to_str ((nwamui_cond_field_t)row_data);
        } else {
            str = _("< No conditions >");
        }
    } else {
        str = nwamui_cond_op_to_str ((nwamui_cond_op_t)row_data);
    }
    g_object_set (G_OBJECT(renderer),
      "text",
      str,
      NULL);
}

static void
_cu_ncu_combo_cell_cb (GtkCellLayout *cell_layout,
  GtkCellRenderer   *renderer,
  GtkTreeModel      *model,
  GtkTreeIter       *iter,
  gpointer           data)
{
    NwamuiNcu  *ncu = NULL;

	gtk_tree_model_get(model, iter, 0, &ncu, -1);

    if ( NWAMUI_NCU(ncu) ) {
        g_object_set (G_OBJECT(renderer),
          "text", nwamui_ncu_get_display_name( ncu ),
          NULL);
        g_object_unref(ncu);
    } else {
        g_object_set (G_OBJECT(renderer), "text", "", NULL);
    }
}

static GtkWidget*
_cu_cond_combo_new (GtkTreeModel *model)
{
    GtkWidget    *combo;
    GtkTreeModel *filter;

    filter = gtk_tree_model_filter_new (model, NULL);
    combo = capplet_compose_combo_with_model(GTK_COMBO_BOX(gtk_combo_box_new()),
      GTK_TREE_MODEL(filter),
      gtk_cell_renderer_combo_new(),
      _cu_cond_combo_cell_cb,
      NULL,
      NULL,
      NULL,
      NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER(filter),
      _cu_cond_combo_filter_visible_cb,
      (gpointer)combo,
      NULL);

    /* Do not refilter now, since the refilter function require check the
     * parent of combo which is currently NULL since we haven't added it to a
     * gtkbox.
     */

    return combo;
}

static void
foreach_nwam_fav_wifi_add_to_combo(gpointer data, gpointer user_data)
{
    gtk_combo_box_append_text(GTK_COMBO_BOX(user_data), nwamui_object_get_name(NWAMUI_OBJECT(data)));
}

static GtkWidget*
_cu_wifi_combo_entry_new ( void )
{
    NwamuiDaemon    *daemon = nwamui_daemon_get_instance();
    GtkWidget       *combo;

    combo = gtk_combo_box_entry_new_text();

    nwamui_daemon_foreach_fav_wifi(daemon, foreach_nwam_fav_wifi_add_to_combo, (gpointer)combo);

    return combo;
}

/**
 * nwam_condition_vbox_new:
 * @returns: a new #NwamConditionVBox.
 *
 * Creates a new #NwamConditionVBox.
 **/
NwamConditionVBox*
nwam_condition_vbox_new (void)
{
    return NWAM_CONDITION_VBOX(g_object_new(NWAM_TYPE_CONDITION_VBOX,
	    "homogeneous", FALSE,
	    "spacing", 8,
	    NULL));
}

static void
table_condition_add_row( gpointer data, gpointer user_data )
{
    NwamConditionVBoxPrivate*   prv = GET_PRIVATE(user_data);
    NwamuiCond*                 cond = NWAMUI_COND(data);
    GtkWidget*                  crow = NULL;
    
    crow = table_condition_insert ( GTK_WIDGET(user_data), 
      prv->table_line_num, cond); 
}

static void
table_models_refresh( void )
{
    NwamuiDaemon   *daemon = nwamui_daemon_get_instance();
    GtkTreeModel   *model;

    /* NCP */
    model = table_get_condition_ncp_list_model ();
    capplet_update_model_from_daemon(model, daemon, NWAMUI_TYPE_NCP);

    /* LOC */
    model = table_get_condition_loc_list_model ();
    capplet_update_model_from_daemon(model, daemon, NWAMUI_TYPE_ENV);

    /* ENM */
    model = table_get_condition_enm_list_model ();
    capplet_update_model_from_daemon(model, daemon, NWAMUI_TYPE_ENM);
}

static void
table_lines_init (NwamConditionVBox *self, NwamuiObject *object)
{
    NwamConditionVBoxPrivate *prv = GET_PRIVATE(self);
    GtkWidget *crow;
    NwamuiCond* cond;
    
    /* cache all, then remove all */
    gtk_container_foreach (GTK_CONTAINER(self),
                           (GtkCallback)table_line_cache_all_cb,
                           (gpointer)self);

    /* Reload models for values */
    table_models_refresh();

    if ( object ) {
        GList*  condition_list = nwamui_object_get_conditions( object );

        /* Only show Location subject selection if not configuring a Location
         * object.
         */
        prv->hide_location = NWAMUI_IS_ENV(object);

        /* for each condition of NwamuiObject; do */
        g_list_foreach( condition_list,
          table_condition_add_row, 
          (gpointer)self);

        nwamui_util_free_obj_list( condition_list );

        /* If there are no rows inserted */
        if (prv->table_line_num == 0) {
            GtkWidget *add;
            GtkWidget *remove;
            cond = nwamui_cond_new();
            nwamui_cond_set_field( cond, NWAMUI_COND_FIELD_LAST  );
            /* If there is no entry, then add one for the "No Conditions" selection */
            crow = table_condition_insert (GTK_WIDGET(self), 0, cond); 
            
            /* Disable add button while No conditions selected */
            add = GTK_WIDGET(g_object_get_data(G_OBJECT(crow),
                                                  TABLE_ROW_ADD));
        }

    }
    /* Update remove buttons */
    gtk_container_foreach(GTK_CONTAINER(self), update_condition_row_widgets, (gpointer)self);

    gtk_widget_show_all (GTK_WIDGET(self));
}

static void
table_lines_free (NwamConditionVBox *self)
{
    NwamConditionVBoxPrivate *prv = GET_PRIVATE(self);
    g_list_foreach (prv->table_box_cache, (GFunc)g_object_unref, NULL);
    g_list_free(prv->table_box_cache);
    prv->table_box_cache = NULL;
}

extern GtkTreeModel *
table_get_condition_subject_model ()
{
    static GtkTreeModel *condition_subject = NULL;
	GtkTreeIter iter;
    gint i;

    if (condition_subject != NULL) {
        return condition_subject;
    }

	condition_subject = GTK_TREE_MODEL(gtk_list_store_new (1, G_TYPE_INT));

    g_object_set_data (G_OBJECT(condition_subject), TABLE_CONDITION_SHOW_NO_COND, (gpointer)TRUE );

	/* Note we use NWAMUI_COND_FIELD_LAST to display <No conditions> */
    /* In spec 1.8.4, there is no <no conditions> contidion. */
    for (i = NWAMUI_COND_FIELD_NCP; i <= NWAMUI_COND_FIELD_LAST; i++) {
        gtk_list_store_append (GTK_LIST_STORE(condition_subject), &iter);
        gtk_list_store_set (GTK_LIST_STORE(condition_subject), &iter, 0, i, -1);
    }
    return condition_subject;
}

extern GtkTreeModel *
table_get_condition_predicate_model ()
{
    static GtkTreeModel *condition_predicate = NULL;
	GtkTreeIter iter;
    gint i;

    if (condition_predicate != NULL) {
        return condition_predicate;
    }

	condition_predicate = GTK_TREE_MODEL(gtk_list_store_new (1, G_TYPE_INT));
	
    for (i = NWAMUI_COND_OP_IS; i < NWAMUI_COND_OP_LAST; i++) {
        gtk_list_store_append (GTK_LIST_STORE(condition_predicate), &iter);
        gtk_list_store_set (GTK_LIST_STORE(condition_predicate), &iter, 0, i, -1);
    }
    return condition_predicate;
}

extern GtkTreeModel *
table_get_condition_ncp_list_model ()
{
    static GtkTreeModel    *model = NULL;

    if ( model == NULL ) {
        model = GTK_TREE_MODEL(gtk_list_store_new(1, NWAMUI_TYPE_NCP, -1 ));
    }

    return (model);
}

extern GtkTreeModel *
table_get_condition_ncu_list_model ()
{
    NwamuiDaemon   *daemon = nwamui_daemon_get_instance();
    NwamuiObject   *active_ncp = nwamui_daemon_get_active_ncp( daemon );

	return (GTK_TREE_MODEL(nwamui_ncp_get_ncu_list_store(NWAMUI_NCP(active_ncp))));
}

extern GtkTreeModel *
table_get_condition_loc_list_model ()
{
    static GtkTreeModel    *model = NULL;

    if ( model == NULL ) {
        model = GTK_TREE_MODEL(gtk_list_store_new(1, NWAMUI_TYPE_ENV, -1 ));
    }

    return (model);
}

extern GtkTreeModel *
table_get_condition_enm_list_model ()
{
    static GtkTreeModel    *model = NULL;

    if ( model == NULL ) {
        model = GTK_TREE_MODEL(gtk_list_store_new(1, NWAMUI_TYPE_ENM, -1 ));
    }

    return (model);
}

static void
select_item_with_value( GtkComboBox*  combo, const gchar* value )
{
    gpointer       *ptr = NULL;
    GtkTreeIter     iter;
    GtkTreeModel   *model;
    gboolean        match = FALSE;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX(combo));

    if ( gtk_tree_model_get_iter_first( model, &iter ) ) {
        gchar  *obj_name = NULL;

        do {
            GObject* obj = NULL;

            gtk_tree_model_get (model, &iter, 0, &ptr, -1);


            if ( gtk_tree_model_get_column_type(model, 0) == G_TYPE_OBJECT 
                 || gtk_tree_model_get_column_type(model, 0) == NWAMUI_TYPE_NCP
                 || gtk_tree_model_get_column_type(model, 0) == NWAMUI_TYPE_NCU
                 || gtk_tree_model_get_column_type(model, 0) == NWAMUI_TYPE_ENM
                 || gtk_tree_model_get_column_type(model, 0) == NWAMUI_TYPE_ENV ) {

                obj = (GObject*)ptr;

                if ( NWAMUI_IS_NCU( obj ) ) {
                    obj_name = nwamui_ncu_get_nwam_qualified_name( NWAMUI_NCU(obj) );
                }
                else if ( NWAMUI_IS_NCP(obj) || NWAMUI_IS_ENV(obj) || NWAMUI_IS_ENM(obj) ) {
                    obj_name = g_strdup(nwamui_object_get_name(NWAMUI_OBJECT(obj)));
                }
            }
            else {
                /* Assume is a string */
                obj_name = g_strdup((gchar*)ptr);
            }

            if ( g_strcmp0(obj_name, value) == 0 ) {
                match = TRUE;
            }
            else if ( obj && NWAMUI_IS_NCU( obj ) ) {
                /* Just to be 100% sure, let's check without qualified name */
                gchar** obj_split = g_strsplit( obj_name?obj_name:"", ":", 2);
                gchar** value_split = g_strsplit( value?value:"", ":", 2);

                if ( obj_split != NULL && value_split != NULL ) {
                    const gchar *obj_str = NULL;
                    const gchar *value_str = NULL;

                    if ( obj_split[0] != NULL && obj_split[1] != NULL ) {
                        obj_str = obj_split[1];
                    }
                    if ( value_split[0] != NULL && value_split[1] != NULL ) {
                        value_str = value_split[1];
                    }
                    if ( g_strcmp0( obj_str, value_str ) == 0 ) {
                        match = TRUE;
                    }
                }
                g_strfreev( obj_split );
                g_strfreev( value_split );
            }
            if ( match ) {
                gtk_combo_box_set_active_iter( combo, &iter );
            }

            if ( obj ) {
                g_object_unref(obj);
            }
            g_free(obj_name);
        }
        while (!match && gtk_tree_model_iter_next(model, &iter));
    }

    if ( !match ) {
        if ( GTK_IS_COMBO_BOX_ENTRY( combo ) ) {
            GtkEntry*   entry = GTK_ENTRY(gtk_bin_get_child( GTK_BIN(combo) ));

            gtk_entry_set_text( entry, value );
        }
        else {
            gtk_combo_box_set_active( combo, 0 );
        }
    }
}

static GtkWidget *
table_condition_new (NwamConditionVBox *self)
{
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(self);
	GtkBox *box = NULL;
	GtkComboBox *combo1, *combo2, *ncp_combo, *ncu_combo, *enm_combo, *loc_combo;
    GtkComboBoxEntry *wifi_combo_entry;
    GtkNotebook *value_nb;
	GtkEntry *entry;
	GtkButton *add, *remove;
	
	if (prv->table_box_cache) {
		box = GTK_BOX(prv->table_box_cache->data);
		prv->table_box_cache = g_list_delete_link(prv->table_box_cache, prv->table_box_cache);
	} else {
		box = GTK_BOX(gtk_hbox_new (FALSE, 2));
		combo1 = GTK_COMBO_BOX(_cu_cond_combo_new (table_get_condition_subject_model()));
		combo2 = GTK_COMBO_BOX(_cu_cond_combo_new (table_get_condition_predicate_model()));
        /* Create a Notebook to handle different widget flavours */
        value_nb = GTK_NOTEBOOK(gtk_notebook_new());
        g_object_set( G_OBJECT(value_nb),
                      "show-tabs", FALSE,
                      "show-border", FALSE,
                      NULL);
        /* VALUE_ENTRY_PAGE  */
		entry = GTK_ENTRY(gtk_entry_new ());
        gtk_notebook_append_page( value_nb, GTK_WIDGET(entry), NULL );
        /* VALUE_NCP_COMBO_PAGE */
		ncp_combo = GTK_COMBO_BOX(capplet_compose_combo_with_model(GTK_COMBO_BOX(gtk_combo_box_new()),
            table_get_condition_ncp_list_model(),
            gtk_cell_renderer_combo_new(),
            nwamui_object_name_cell,
            NULL,
		    G_CALLBACK(condition_gobject_combo_changed_cb),
		    (gpointer)box,
            NULL));
        gtk_notebook_append_page( value_nb, GTK_WIDGET(ncp_combo), NULL );
        /* VALUE_NCU_COMBO_PAGE */
		ncu_combo = GTK_COMBO_BOX(capplet_compose_combo_with_model(GTK_COMBO_BOX(gtk_combo_box_new()),
            table_get_condition_ncu_list_model(),
            gtk_cell_renderer_combo_new(),
            _cu_ncu_combo_cell_cb,
            NULL,
		    G_CALLBACK(condition_gobject_combo_changed_cb),
		    (gpointer)box,
            NULL));
        gtk_notebook_append_page( value_nb, GTK_WIDGET(ncu_combo), NULL );
        /* VALUE_ENM_COMBO_PAGE */
		enm_combo = GTK_COMBO_BOX(capplet_compose_combo_with_model(GTK_COMBO_BOX(gtk_combo_box_new()),
            table_get_condition_enm_list_model(),
            gtk_cell_renderer_combo_new(),
            nwamui_object_name_cell,
            NULL,
		    G_CALLBACK(condition_gobject_combo_changed_cb),
		    (gpointer)box,
            NULL));
        gtk_notebook_append_page( value_nb, GTK_WIDGET(enm_combo), NULL );
        /* VALUE_LOC_COMBO_PAGE */
		loc_combo = GTK_COMBO_BOX(capplet_compose_combo_with_model(GTK_COMBO_BOX(gtk_combo_box_new()),
            table_get_condition_loc_list_model(),
            gtk_cell_renderer_combo_new(),
            nwamui_object_name_cell,
            NULL,
		    G_CALLBACK(condition_gobject_combo_changed_cb),
		    (gpointer)box,
            NULL));
        gtk_notebook_append_page( value_nb, GTK_WIDGET(loc_combo), NULL );
        /* VALUE_WIFI_COMBO_ENTRY_PAGE */
		wifi_combo_entry = GTK_COMBO_BOX_ENTRY(_cu_wifi_combo_entry_new());
        gtk_notebook_append_page( value_nb, GTK_WIDGET(wifi_combo_entry), NULL );
    
        g_object_set(G_OBJECT(entry),
                    "width-chars", 20,
                    NULL);

		g_signal_connect(combo1, "changed",
          G_CALLBACK(condition_field_op_changed_cb),
		    (gpointer)box);

		g_signal_connect(combo2, "changed",
		    G_CALLBACK(condition_field_op_changed_cb),
		    (gpointer)box);

		g_signal_connect(wifi_combo_entry, "changed",
		    G_CALLBACK(condition_wifi_combo_changed_cb),
		    (gpointer)box);

		g_signal_connect(entry, "changed",
		    G_CALLBACK(condition_value_changed_cb),
		    (gpointer)box);

		add = GTK_BUTTON(gtk_button_new());
		remove = GTK_BUTTON(gtk_button_new ());
		gtk_button_set_image (add, gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
		gtk_button_set_image (remove, gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));
		g_signal_connect(add, "clicked",
		    G_CALLBACK(table_add_condition_cb),
		    (gpointer)self);
		g_signal_connect(remove, "clicked",
		    G_CALLBACK(table_delete_condition_cb),
		    (gpointer)self);
		gtk_box_pack_start (box, GTK_WIDGET(combo1), TRUE, TRUE, 0);
		gtk_box_pack_start (box, GTK_WIDGET(combo2), TRUE, TRUE, 0);
		gtk_box_pack_start (box, GTK_WIDGET(value_nb), TRUE, TRUE, 0);
		gtk_box_pack_start (box, GTK_WIDGET(add), FALSE, FALSE, 0);
		gtk_box_pack_start (box, GTK_WIDGET(remove), FALSE, FALSE, 0);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_COMBO1, (gpointer)combo1);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_COMBO2, (gpointer)combo2);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_NOTEBOOK, (gpointer)value_nb );
		g_object_set_data (G_OBJECT(box), TABLE_ROW_ENTRY, (gpointer)entry);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_NCP_COMBO, (gpointer)ncp_combo);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_NCU_COMBO, (gpointer)ncu_combo);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_ENM_COMBO, (gpointer)enm_combo);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_LOC_COMBO, (gpointer)loc_combo);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_WIFI_COMBO_ENTRY, (gpointer)wifi_combo_entry);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_ADD, (gpointer)add);
		g_object_set_data (G_OBJECT(box), TABLE_ROW_REMOVE, (gpointer)remove);
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo1), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo2), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(ncp_combo), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(ncu_combo), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(enm_combo), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(loc_combo), 0);
	}
    gtk_widget_show_all (GTK_WIDGET(box));
	return GTK_WIDGET(box);
}

static void
table_line_cache_all_cb (GtkWidget *widget, gpointer data)
{
    GtkWidget *clist = GTK_WIDGET (data);
    /* cache the remove one */
    table_condition_delete(clist, widget);
}

static GtkWidget*
table_condition_insert (GtkWidget *clist, guint row, NwamuiCond* cond)
{
	gpointer self = clist;
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(self);
	GtkComboBox *combo1, *combo2, *ncp_combo, *ncu_combo, *enm_combo, *loc_combo;
    GtkComboBoxEntry *wifi_combo_entry;
    GtkNotebook *value_nb;
	GtkEntry *entry;
	GtkButton *add, *remove;
	GtkWidget *box;

	box = table_condition_new (NWAM_CONDITION_VBOX(self));
	gtk_box_pack_start (GTK_BOX(clist), box, FALSE, FALSE, 0);
	prv->table_line_num++;
	gtk_box_reorder_child (GTK_BOX(clist), box, row);
	gtk_widget_show_all (clist);
	nwamui_debug ("num %d",     prv->table_line_num);

    /* Update the row widgets */
    combo1 = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_COMBO1));
    combo2 = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_COMBO2));
    value_nb = GTK_NOTEBOOK(g_object_get_data(G_OBJECT(box), TABLE_ROW_NOTEBOOK ));
    entry = GTK_ENTRY(g_object_get_data(G_OBJECT(box), TABLE_ROW_ENTRY));
    ncp_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_NCP_COMBO));
    ncu_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_NCU_COMBO));
    enm_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_ENM_COMBO));
    loc_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_LOC_COMBO));
    wifi_combo_entry = GTK_COMBO_BOX_ENTRY(g_object_get_data(G_OBJECT(box), TABLE_ROW_WIFI_COMBO_ENTRY));
    add = GTK_BUTTON(g_object_get_data(G_OBJECT(box), TABLE_ROW_ADD));
    remove = GTK_BUTTON(g_object_get_data(G_OBJECT(box), TABLE_ROW_REMOVE));

	g_object_set_data (G_OBJECT(box), TABLE_ROW_CDATA, g_object_ref(cond) );
	
    gtk_widget_show_all( GTK_WIDGET(box) );

	if (cond && NWAMUI_IS_COND( cond )) {
		// Initialize box according to data
        GtkTreeModel       *model = NULL;
		nwamui_cond_field_t field = nwamui_cond_get_field( cond );
		nwamui_cond_op_t    oper  = nwamui_cond_get_oper( cond );
		gchar*              value = nwamui_cond_get_value( cond );

        gtk_combo_box_set_active(combo1, -1);
        gtk_combo_box_set_active(combo2, -1);
        set_combobox_unfiltered_index( GTK_COMBO_BOX(combo1), (gint)field);
        set_combobox_unfiltered_index( GTK_COMBO_BOX(combo2), (gint)oper);

        if (field == NWAMUI_COND_FIELD_NCP) {
            gtk_notebook_set_current_page( value_nb, VALUE_NCP_COMBO_PAGE );
            /* Find match for NCP in available list */
            select_item_with_value( ncp_combo, value );
        }
        else if (field == NWAMUI_COND_FIELD_NCU) {
            gtk_notebook_set_current_page( value_nb, VALUE_NCU_COMBO_PAGE );
            /* Find match for NCU in available list */
            select_item_with_value( ncu_combo, value );
        }
        else if (field == NWAMUI_COND_FIELD_LOC) {
            gtk_notebook_set_current_page( value_nb, VALUE_LOC_COMBO_PAGE );
            /* Find match for LOC in available list */
            select_item_with_value( loc_combo, value );
        }
        else if (field == NWAMUI_COND_FIELD_ENM ) {
            gtk_notebook_set_current_page( value_nb, VALUE_ENM_COMBO_PAGE );
            /* Find match for ENM in available list */
            select_item_with_value( enm_combo, value );
        }
        else if (field == NWAMUI_COND_FIELD_ESSID ) {
            GtkComboBox *combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(box), TABLE_ROW_WIFI_COMBO_ENTRY));
            gtk_notebook_set_current_page( value_nb, VALUE_WIFI_COMBO_ENTRY_PAGE );
            /* Find match for ESSID in available list */
            select_item_with_value( GTK_COMBO_BOX(wifi_combo_entry), value );
        }
        else {
            gtk_notebook_set_current_page( value_nb, VALUE_ENTRY_PAGE );
            gtk_entry_set_text (GTK_ENTRY(entry), value?value:"" );
        }

        g_free(value);
	} else {
		/* default initialize box */
        set_combobox_unfiltered_index( GTK_COMBO_BOX(combo1), (gint)NWAMUI_COND_FIELD_LAST );
		gtk_entry_set_text (GTK_ENTRY(entry), "");
        gtk_notebook_set_current_page( value_nb, VALUE_ENTRY_PAGE );
        gtk_combo_box_set_active( ncu_combo, 0 );
        gtk_combo_box_set_active( enm_combo, 0 );
        gtk_combo_box_set_active( loc_combo, 0 );
        gtk_combo_box_set_active( GTK_COMBO_BOX(wifi_combo_entry), 0 );
	}

    g_signal_emit (self,
      cond_signals[S_CONDITION_ADD],
      0, /* details */
      cond);

	return box;
}

static void
update_condition_row_widgets( GtkWidget* widget, gpointer user_data )
{
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(user_data);
    GtkWidget  *combo1, *combo2, *remove;
    nwamui_cond_field_t  field;

	combo1 = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), TABLE_ROW_COMBO1));
	combo2 = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), TABLE_ROW_COMBO2));
    remove = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), TABLE_ROW_REMOVE));

    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_combo_box_get_model(GTK_COMBO_BOX(combo1))));

    field = (nwamui_cond_field_t)get_unfiltered_index_from_combo(GTK_COMBO_BOX(combo1));

    if ( field != NWAMUI_COND_FIELD_LAST ) {
        gtk_widget_set_sensitive(combo2, TRUE);
    } else {
        gtk_widget_set_sensitive(combo2, FALSE);
    }

    gtk_widget_set_sensitive(remove, (prv->table_line_num > 1));
}

static void
table_condition_delete(GtkWidget *clist, GtkWidget *crow)
{
	gpointer self = clist;
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(self);
    NwamuiCond* cond = NULL;

	/* cache the remove one */
	prv->table_box_cache = g_list_prepend (prv->table_box_cache,
	    g_object_ref (G_OBJECT(crow)));
	gtk_container_remove (GTK_CONTAINER(clist), crow);
	prv->table_line_num--;
	gtk_widget_show_all (clist);
	nwamui_debug ("num %d",     prv->table_line_num);

    cond = NWAMUI_COND(g_object_get_data (G_OBJECT(crow), TABLE_ROW_CDATA));
    g_signal_emit (self,
         cond_signals[S_CONDITION_REMOVE],
         0, /* details */
         cond);
}

static void
table_add_condition_cb (GtkButton *button, gpointer data)
{
    /* a crow is condition row, a clist is an set of crows */
    GtkWidget *crow = GTK_WIDGET(gtk_widget_get_parent (GTK_WIDGET(button)));
    GtkWidget *clist = gtk_widget_get_parent (crow);
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(data);
	guint row;
    NwamuiCond* cond = NULL;

    gtk_container_child_get (GTK_CONTAINER(clist),
                             crow,
                             "position", &row,
                             NULL);

    cond = nwamui_cond_new();
    crow = table_condition_insert (clist, row + 1, cond ); 
    gtk_container_foreach( GTK_CONTAINER(clist), update_condition_row_widgets, clist );
}

static void
table_delete_condition_cb(GtkButton *button, gpointer data)
{
    /* a crow is condition row, a clist is an set of crows */
    GtkWidget *crow = GTK_WIDGET(gtk_widget_get_parent (GTK_WIDGET(button)));
    GtkWidget *clist = gtk_widget_get_parent (crow);
	NwamConditionVBoxPrivate *prv = GET_PRIVATE(data);
	guint row;
    NwamuiCond* cond = NULL;

    gtk_container_child_get(GTK_CONTAINER(clist),
      crow,
      "position", &row,
      NULL);

    table_condition_delete(clist, crow);
    
    if (prv->table_line_num == 0) {
        GtkWidget *remove;
        cond = nwamui_cond_new();
        /* Ensure we have an entry for the "No Conditions" selection. */
        crow = table_condition_insert (clist, 0, cond); 
    }
    gtk_container_foreach( GTK_CONTAINER(clist), update_condition_row_widgets, clist );
}

static void
condition_field_op_changed_cb( GtkWidget* widget, gpointer data )
{
    NwamuiCond* cond = NWAMUI_COND(g_object_get_data (G_OBJECT(data), TABLE_ROW_CDATA));
    gint index;
    GtkTreeIter iter, realiter;
    GtkTreeModelFilter *filter;
    GtkTreeModel *model;

    if (!cond)
        return;

    filter = GTK_TREE_MODEL_FILTER(gtk_combo_box_get_model (GTK_COMBO_BOX(widget)));
    model = gtk_tree_model_filter_get_model (filter);

    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX(widget), &iter)) {
        gboolean    no_conditions = TRUE;

        gtk_tree_model_filter_convert_iter_to_child_iter (filter,
                                                          &realiter, &iter);
        gtk_tree_model_get (model, &realiter, 0, &index, -1);

        if (model == table_get_condition_subject_model ()) {
	        GtkEntry            *entry            = GTK_ENTRY(g_object_get_data(G_OBJECT(data), TABLE_ROW_ENTRY));
            GtkComboBox         *combo2           = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(data), TABLE_ROW_COMBO2));
		    GtkNotebook         *value_nb         = GTK_NOTEBOOK(g_object_get_data(G_OBJECT(data), TABLE_ROW_NOTEBOOK ));
		    GtkComboBox         *ncp_combo        = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(data), TABLE_ROW_NCP_COMBO));
		    GtkComboBox         *ncu_combo        = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(data), TABLE_ROW_NCU_COMBO));
		    GtkComboBox         *enm_combo        = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(data), TABLE_ROW_ENM_COMBO));
		    GtkComboBox         *loc_combo        = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(data), TABLE_ROW_LOC_COMBO));
		    GtkComboBox         *wifi_combo_entry = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(data), TABLE_ROW_WIFI_COMBO_ENTRY));
            GtkWidget           *add              = GTK_WIDGET(g_object_get_data(G_OBJECT(data), TABLE_ROW_ADD));
            nwamui_cond_field_t  field;
            gchar*               value;
            /*
             * work out thec contents depends on the selection
             * of the first combo.
             */
            _cu_cond_combo_filter_value (GTK_WIDGET(combo2), index);
            gtk_combo_box_set_active( combo2, 0 );
            field = (nwamui_cond_field_t)index;

            nwamui_cond_set_field( cond, (nwamui_cond_field_t)index);

            if ( field != NWAMUI_COND_FIELD_LAST ) {
                no_conditions = FALSE;
                value = nwamui_cond_get_value( cond );
            }
            else { 
                value = g_strdup( "" );
            }

            /* Disable add button while No conditions selected */
            gtk_widget_set_sensitive (add, !no_conditions );
            /* Update combo2 */
            gtk_widget_set_sensitive( GTK_WIDGET(combo2), !no_conditions);

            if (field == NWAMUI_COND_FIELD_NCP) {
                gtk_notebook_set_current_page( value_nb, VALUE_NCP_COMBO_PAGE );
                /* Find match for NCP in available list */
                select_item_with_value( ncp_combo, value );
                gtk_widget_set_sensitive( GTK_WIDGET(ncp_combo), !no_conditions);
            }
            else if (field == NWAMUI_COND_FIELD_NCU) {
                gtk_notebook_set_current_page( value_nb, VALUE_NCU_COMBO_PAGE );
                /* Find match for NCU in available list */
                select_item_with_value( ncu_combo, value );
                gtk_widget_set_sensitive( GTK_WIDGET(ncu_combo), !no_conditions);
            }
            else if (field == NWAMUI_COND_FIELD_LOC) {
                gtk_notebook_set_current_page( value_nb, VALUE_LOC_COMBO_PAGE );
                /* Find match for LOC in available list */
                select_item_with_value( loc_combo, value );
                gtk_widget_set_sensitive( GTK_WIDGET(loc_combo), !no_conditions);
            }
            else if (field == NWAMUI_COND_FIELD_ENM ) {
                gtk_notebook_set_current_page( value_nb, VALUE_ENM_COMBO_PAGE );
                /* Find match for ENM in available list */
                select_item_with_value( enm_combo, value );
                gtk_widget_set_sensitive( GTK_WIDGET(enm_combo), !no_conditions);
            }
            else if (field == NWAMUI_COND_FIELD_ESSID ) {
                gtk_notebook_set_current_page( value_nb, VALUE_WIFI_COMBO_ENTRY_PAGE );
                /* Find match for ESSID in available list */
                select_item_with_value( GTK_COMBO_BOX(wifi_combo_entry), value );
                gtk_widget_set_sensitive( GTK_WIDGET(wifi_combo_entry), !no_conditions);
            }
            else {
                gtk_widget_set_sensitive( GTK_WIDGET(entry), !no_conditions);
                if (field == NWAMUI_COND_FIELD_IP_ADDRESS) {
                    nwamui_cond_op_t                op;
                    nwamui_entry_validation_flags_t flags;

                    flags = NWAMUI_ENTRY_VALIDATION_IS_V4|
                            NWAMUI_ENTRY_VALIDATION_IS_V6|
                            NWAMUI_ENTRY_VALIDATION_ALLOW_EMPTY;

                    op = nwamui_cond_get_oper( cond );
                    switch ( op ) {
                    case NWAMUI_COND_OP_IS_IN_RANGE:
                    case NWAMUI_COND_OP_IS_NOT_IN_RANGE:
                        flags |= NWAMUI_ENTRY_VALIDATION_ALLOW_PREFIX;
                    default:
                        break;
                    }
                    nwamui_util_set_entry_validation( GTK_ENTRY(entry), flags, TRUE );
                }
                else if ( nwamui_cond_get_field( cond ) == NWAMUI_COND_FIELD_BSSID) {
                    nwamui_util_set_entry_validation( GTK_ENTRY(entry), 
                                                      NWAMUI_ENTRY_VALIDATION_IS_ETHERS|NWAMUI_ENTRY_VALIDATION_ALLOW_EMPTY, TRUE );
                }
                else {
                    nwamui_util_unset_entry_validation( GTK_ENTRY(entry) );
                    gtk_entry_set_text( GTK_ENTRY(entry), "" );
                }
                gtk_notebook_set_current_page( value_nb, VALUE_ENTRY_PAGE );
                gtk_entry_set_text (GTK_ENTRY(entry), value?value:"" );
            }
            g_free(value);
        } else {
            nwamui_cond_op_t                op = (nwamui_cond_op_t)index;

            if ( nwamui_cond_get_field( cond ) == NWAMUI_COND_FIELD_IP_ADDRESS) {
                GtkEntry   *entry = GTK_ENTRY(g_object_get_data(G_OBJECT(data), TABLE_ROW_ENTRY));
                nwamui_entry_validation_flags_t flags;

                flags = NWAMUI_ENTRY_VALIDATION_IS_V4|
                        NWAMUI_ENTRY_VALIDATION_IS_V6|
                        NWAMUI_ENTRY_VALIDATION_ALLOW_EMPTY;


                switch ( op ) {
                case NWAMUI_COND_OP_IS_IN_RANGE:
                case NWAMUI_COND_OP_IS_NOT_IN_RANGE:
                    flags |= NWAMUI_ENTRY_VALIDATION_ALLOW_PREFIX;
                default:
                    break;
                }
                nwamui_util_set_entry_validation_flags( GTK_ENTRY(entry), flags );
            }
            else if ( nwamui_cond_get_field( cond ) == NWAMUI_COND_FIELD_BSSID) {
                GtkEntry   *entry = GTK_ENTRY(g_object_get_data(G_OBJECT(data), TABLE_ROW_ENTRY));

                nwamui_util_set_entry_validation_flags( GTK_ENTRY(entry), NWAMUI_ENTRY_VALIDATION_IS_ETHERS|NWAMUI_ENTRY_VALIDATION_ALLOW_EMPTY ) ;
            }

            nwamui_cond_set_oper( cond, op);
        }
    }
}

static void
condition_wifi_combo_changed_cb( GtkComboBox* combo, gpointer data )
{
    NwamuiCond *cond = NWAMUI_COND(g_object_get_data (G_OBJECT(data), TABLE_ROW_CDATA));
    gchar      *value = NULL;

    if (!cond)
        return;

    value = gtk_combo_box_get_active_text( combo );

    if ( cond != NULL ) {
        nwamui_cond_set_value(cond, value);
    }

    g_free(value);
}

static void
condition_gobject_combo_changed_cb( GtkComboBox* combo, gpointer data )
{
    NwamuiCond* cond = NWAMUI_COND(g_object_get_data (G_OBJECT(data), TABLE_ROW_CDATA));
    GObject*    obj = NULL;
    GtkTreeIter iter, realiter;
    GtkTreeModelFilter *filter;
    GtkTreeModel *model;
    gchar              *value = NULL;

    if (!cond)
        return;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX(combo));

    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX(combo), &iter)) {
        gtk_tree_model_get (model, &iter, 0, &obj, -1);

        if ( NWAMUI_IS_NCU( obj ) ) {
            value = nwamui_ncu_get_device_name( NWAMUI_NCU(obj) );
        }
        else if ( NWAMUI_IS_NCP(obj)  || NWAMUI_IS_ENV(obj) || NWAMUI_IS_ENM(obj) ) {
            value = g_strdup(nwamui_object_get_name(NWAMUI_OBJECT(obj)));
        }
        nwamui_cond_set_value(cond, value);
        g_object_unref(obj);
    }

    g_free(value);
}

static void
condition_value_changed_cb( GtkWidget* widget, gpointer data )
{
    NwamuiCond             *cond = NWAMUI_COND(g_object_get_data (G_OBJECT(data), TABLE_ROW_CDATA));
    GtkComboBox            *combo1 = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(data), TABLE_ROW_COMBO1));
    nwamui_cond_field_t     field;
    gboolean                valid = FALSE;
    const gchar            *text = NULL;;

    if (!cond)
        return;

    if ( GTK_IS_ENTRY(widget) ) {
        text = gtk_entry_get_text(GTK_ENTRY(widget));
    }

    field = (nwamui_cond_field_t)get_unfiltered_index_from_combo( combo1 );
    
    /* Need to validate the text appropriately w.r.t. the field selected */
    switch ( field ) {
        case NWAMUI_COND_FIELD_NCP: {
                valid = TRUE;
            }
            break;
        case NWAMUI_COND_FIELD_NCU: {
                valid = TRUE;
            }
            break;
        case NWAMUI_COND_FIELD_ENM: {
                valid = TRUE;
            }
            break;
        case NWAMUI_COND_FIELD_LOC: {
                valid = TRUE;
            }
            break;
        case NWAMUI_COND_FIELD_IP_ADDRESS: {
                valid = TRUE;
            }
            break;
        case NWAMUI_COND_FIELD_ADV_DOMAIN: {
                valid = TRUE;
            }
            break;
        case NWAMUI_COND_FIELD_SYS_DOMAIN: {
                valid = TRUE;
            }
            break;
        case NWAMUI_COND_FIELD_ESSID: {
                valid = TRUE;
            }
            break;
        case NWAMUI_COND_FIELD_BSSID: {
                valid = TRUE;
            }
            break;
    }

    if ( valid ) {
        nwamui_cond_set_value(cond, gtk_entry_get_text(GTK_ENTRY(widget)));
    }
}

static void
default_condition_add_signal_handler (NwamConditionVBox *self, GObject* data, gpointer user_data)
{
    g_debug("Condition Add : 0x%p", data);
}

static void
default_condition_remove_signal_handler (NwamConditionVBox *self, GObject* data, gpointer user_data)
{
    g_debug("Condition Remove : 0x%p", data);
}

