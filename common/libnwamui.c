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
 * File:   libnwamui.c
 *
 * Created on May 23, 2007, 10:26 AM
 * 
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <libnwamui.h>
#include <glade/glade.h>
#include <libgnome/libgnome.h>
#include <string.h>

#define NWAM_MANAGER_PROPERTIES_GLADE_FILE  "nwam-manager-properties.glade"

#define NWAM_ENVIRONMENT_RENAME     "nwam_environment_rename"
#define RENAME_ENVIRONMENT_ENTRY    "rename_environment_entry"

struct nwamui_wifi_essid {
    gchar*                          essid;
    nwamui_wifi_security_t          security;
    gchar*                          bssid;
    nwamui_wifi_signal_strength_t   signal_strength;
    nwamui_wifi_wpa_config_t        wpa_config;
};

typedef struct nwamui_wifi_essid nwamui_wifi_essid_t;


/* Load the Glade file and maintain a single reference */
static GladeXML *glade_xml_tree = NULL;

static GladeXML* 
get_glade_xml( void ) {
    static const gchar *build_datadir = NWAM_MANAGER_DATADIR;

    if ( glade_xml_tree == NULL ) {
        const gchar * const *sys_data_dirs;
        const gchar *glade_file;
        gint i;

        sys_data_dirs = g_get_system_data_dirs ();
        if ( g_file_test(build_datadir, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR) ) {
            gchar *glade_file;
            /* First try with the package name in the path */
            glade_file = g_build_filename (build_datadir, PACKAGE, NWAM_MANAGER_PROPERTIES_GLADE_FILE, NULL );
            g_debug("Attempting to load : %s", glade_file);
            if ( (glade_xml_tree = glade_xml_new(glade_file, NULL, NULL)) != NULL ) {
                g_debug("Found glade file at : %s", glade_file );
            }
            g_free (glade_file);
            if ( glade_xml_tree == NULL ) {
                /* Now try without it */
                glade_file = g_build_filename (build_datadir, NWAM_MANAGER_PROPERTIES_GLADE_FILE, NULL );
                g_debug("Attempting to load : %s", glade_file);
                if ( (glade_xml_tree = glade_xml_new(glade_file, NULL, NULL)) != NULL ) {
                    g_debug("Found glade file at : %s", glade_file );
                    g_free (glade_file);
                }
            }
        }
        if (glade_xml_tree == NULL ) {
            for (i = 0; sys_data_dirs[i] != NULL; i++) {
                gchar *glade_file;
                /* First try with the package name in the path */
                glade_file = g_build_filename (sys_data_dirs[i], PACKAGE, NWAM_MANAGER_PROPERTIES_GLADE_FILE, NULL );
                g_debug("Attempting to load : %s", glade_file);
                if ( (glade_xml_tree = glade_xml_new(glade_file, NULL, NULL)) != NULL ) {
                    g_debug("Found glade file at : %s", glade_file );
                    break;
                }
                g_free (glade_file);
                /* Now try without it */
                glade_file = g_build_filename (sys_data_dirs[i], NWAM_MANAGER_PROPERTIES_GLADE_FILE, NULL );
                g_debug("Attempting to load : %s", glade_file);
                if ( (glade_xml_tree = glade_xml_new(glade_file, NULL, NULL)) != NULL ) {
                    g_debug("Found glade file at : %s", glade_file );
                    g_free (glade_file);
                    break;
                }
            }
        }

        g_assert( glade_xml_tree != NULL );
    }
    return glade_xml_tree;
}

/**
 * nwamui_util_glade_get_widget:
 * @widget_name: name of the widget to load.
 * @returns: the widget loaded from the Glade file.
 *
 **/
extern GtkWidget*
nwamui_util_glade_get_widget( const gchar* widget_name ) 
{
    GladeXML*   xml;
    GtkWidget*  widget;

    g_assert( widget_name != NULL );
    
    g_return_val_if_fail( widget_name != NULL, NULL );
    
    xml = get_glade_xml();
    g_return_val_if_fail( xml != NULL, NULL );
    
    widget = glade_xml_get_widget(xml, widget_name );
    
    if ( widget == NULL )
        g_debug("Failed to get widget by name %s", widget_name );
    
    g_assert( widget != NULL );
    
    return widget;
}
        
/**
 * nwamui_util_wifi_sec_to_string:
 * @wireless_sec: a #nwamui_wifi_security_t.
 * @returns: a localized string representation of the security type
 *
 **/
extern gchar*
nwamui_util_wifi_sec_to_string( nwamui_wifi_security_t wireless_sec ) {
    g_assert( wireless_sec >= 0 && wireless_sec < NWAMUI_WIFI_SEC_LAST );
    
    switch( wireless_sec ) {
        case NWAMUI_WIFI_SEC_WEP_HEX:           return(_("WEP Hex"));
        case NWAMUI_WIFI_SEC_WEP_ASCII:         return(_("WEP ASCII"));
        case NWAMUI_WIFI_SEC_WPA_PERSONAL:      return(_("WPA Personal(PSK)"));
        case NWAMUI_WIFI_SEC_WPA_ENTERPRISE:    return(_("WPA Enterprise (Radius)"));
        default:                                return(_("None") );
    }
}

/**
 * nwamui_util_wifi_sec_to_short_string:
 * @wireless_sec: a #nwamui_wifi_security_t.
 * @returns: a short localized string representation of the security type
 *
 **/
extern gchar*
nwamui_util_wifi_sec_to_short_string( nwamui_wifi_security_t wireless_sec ) {
    g_assert( wireless_sec >= 0 && wireless_sec < NWAMUI_WIFI_SEC_LAST );
    
    switch( wireless_sec ) {
        case NWAMUI_WIFI_SEC_WEP_HEX:           return(_("WEP"));
        case NWAMUI_WIFI_SEC_WEP_ASCII:         return(_("WEP"));
        case NWAMUI_WIFI_SEC_WPA_PERSONAL:      return(_("WPA"));
        case NWAMUI_WIFI_SEC_WPA_ENTERPRISE:    return(_("WPA"));
        default:                                return(_("Open") );
    }
}


/**
 * nwamui_util_wifi_wpa_config_to_string:
 * @wpa_config: a #nwamui_wifi_wpa_config_t.
 * @returns: a localized string representation of the WPA Config mechanism.
 *
 **/
extern gchar*
nwamui_util_wifi_wpa_config_to_string( nwamui_wifi_wpa_config_t wpa_config ) {
    g_assert( wpa_config >= 0 && wpa_config < NWAMUI_WIFI_WPA_CONFIG_LAST );
    
    switch( wpa_config ) {
        case NWAMUI_WIFI_WPA_CONFIG_LEAP:  	return(_("LEAP")); 
        case NWAMUI_WIFI_WPA_CONFIG_PEAP:  	return(_("PEAP/MSCHAPv2")); 
        case NWAMUI_WIFI_WPA_CONFIG_AUTOMATIC:	/*fall through*/
        default:                                return(_("Automatic"));
    }
}

/**
 * nwamui_util_obj_ref
 *
 * Utility function, for use in g_list_foreach calls to g_object_ref each element.
 *
 **/
extern void
nwamui_util_obj_ref( gpointer obj, gpointer user_data )
{
    g_return_if_fail( G_IS_OBJECT(obj));
    
    g_object_ref( G_OBJECT(obj));
}

/**
 * nwamui_util_obj_unref
 *
 * Utility function, for use in g_list_foreach calls to g_object_unref each element.
 *
 **/
extern void
nwamui_util_obj_unref( gpointer obj, gpointer user_data )
{
    g_return_if_fail( G_IS_OBJECT(obj));
    
    g_object_unref( G_OBJECT(obj));
}


/**
 * nwamui_util_free_obj_list
 *
 * Utility function, given a GList of objects, will unref each and then free the list.
 *
 **/
extern void
nwamui_util_free_obj_list( GList*   obj_list )
{
    g_return_if_fail( obj_list != NULL );
    
    g_list_foreach(obj_list, nwamui_util_obj_unref, NULL );
    
    g_list_free( obj_list );
}

/**
 * nwamui_util_copy_obj_list
 *
 * Utility function, given a GList of objects, will duplicate the list and then ref() each element.
 *
 **/
extern GList*
nwamui_util_copy_obj_list( GList*   obj_list )
{
    GList*  new_list = NULL;
    
    g_return_val_if_fail( obj_list != NULL, NULL );
    
    new_list = g_list_copy( obj_list );
    
    g_list_foreach(new_list, nwamui_util_obj_ref, NULL );    
}

static gint small_icon_size = -1;
static gint normal_icon_size = -1;

static GdkPixbuf*   
get_pixbuf( const gchar* stock_id, gboolean small )
{
    GtkIconTheme*   icon_theme = gtk_icon_theme_get_default();
    GdkPixbuf*      pixbuf = NULL;
    GError*         error = NULL;

    g_debug("get_pixbuf: Seeking %s stock_id = %s", small?"small":"normal", stock_id );

    if ( small_icon_size == -1 ) {
        gint dummy;
        if ( !gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &small_icon_size, &dummy) ) {
            small_icon_size=16;
        }
        if ( !gtk_icon_size_lookup(GTK_ICON_SIZE_LARGE_TOOLBAR, &normal_icon_size, &dummy) ) {
            normal_icon_size=24;
        }
        g_debug("get_pixbuf: small_icon_size = %d", small_icon_size );
        g_debug("get_pixbuf: normal_icon_size = %d", normal_icon_size );
    }
    
    pixbuf = gtk_icon_theme_load_icon( icon_theme, stock_id, 
                                       small?small_icon_size:normal_icon_size, 0, &error );

    if ( pixbuf == NULL ) {
        g_debug("get_pixbuf: pixbuf = NULL");
    }
    else {
        g_debug("get_pixbuf: pixbuf loaded");
    }
    return( pixbuf );
}

/* 
 * Returns a GdkPixbuf that reflects the status of the environment
 */
extern GdkPixbuf*
nwamui_util_get_env_status_icon( nwamui_env_status_t env_status )
{
    GdkPixbuf* icon = NULL;
    
    switch( env_status ) {
        case NWAMUI_ENV_STATUS_CONNECTED:
            icon = get_pixbuf(NWAM_ICON_EARTH_CONNECTED, FALSE);
            break;
        case NWAMUI_ENV_STATUS_WARNING:
            icon = get_pixbuf(NWAM_ICON_EARTH_WARNING, FALSE);
            break;
        case NWAMUI_ENV_STATUS_ERROR:
            icon = get_pixbuf(NWAM_ICON_EARTH_ERROR, FALSE);
            break;
        default:
            g_assert_not_reached();
            break;
    }
    
    return( icon );
}

/* 
 * Returns a GdkPixbuf that reflects the type of the network.
 */
extern GdkPixbuf*
nwamui_util_get_network_type_icon( nwamui_ncu_type_t ncu_type )
{
        static GdkPixbuf       *wireless_icon = NULL;
        static GdkPixbuf       *wired_icon = NULL;

        if ( wireless_icon == NULL ) {
            wireless_icon = get_pixbuf("network-wireless", FALSE);
        }
        if (wired_icon == NULL ) {
            wired_icon = get_pixbuf("network-idle", FALSE);
        }
        
        switch (ncu_type) {
            case NWAMUI_NCU_TYPE_WIRELESS:
                return( GDK_PIXBUF(g_object_ref(G_OBJECT(wireless_icon) )) );
            case NWAMUI_NCU_TYPE_WIRED: 
                /* Fall-through */
            default:
                return( GDK_PIXBUF(g_object_ref(G_OBJECT(wired_icon) )) );
        }
}
       
/* 
 * Returns a GdkPixbuf that reflects the security type of the network.
 */
extern GdkPixbuf*
nwamui_util_get_network_security_icon( nwamui_wifi_security_t sec_type, gboolean small )
{
    static GdkPixbuf       *secured_icon = NULL;
    static GdkPixbuf       *open_icon = NULL;

    /* TODO - get REAL icons for drop-downlist, these are borrowed */
    if ( secured_icon == NULL ) {
        secured_icon = get_pixbuf(NWAM_ICON_NETWORK_SECURE, small);
    }
    if ( open_icon == NULL ) {
        open_icon = get_pixbuf(NWAM_ICON_NETWORK_INSECURE, small);
    }

    switch (sec_type) {
        case NWAMUI_WIFI_SEC_WEP_ASCII:
        case NWAMUI_WIFI_SEC_WEP_HEX:
        case NWAMUI_WIFI_SEC_WPA_ENTERPRISE:
        case NWAMUI_WIFI_SEC_WPA_PERSONAL:
            return( GDK_PIXBUF(g_object_ref(G_OBJECT(secured_icon) )) );
        case NWAMUI_WIFI_SEC_NONE: 
            /* Fall-through */
        default:
            return( GDK_PIXBUF(g_object_ref(G_OBJECT(open_icon) )) );
    }
}
       
/* 
 * Returns a GdkPixbuf that reflects the status of the network.
 */
extern GdkPixbuf*
nwamui_util_get_network_status_icon( NwamuiNcu* ncu )
{
        static GdkPixbuf       *enabled_wired_icon = NULL;
        static GdkPixbuf       *enabled_wireless_icon = NULL;
        static GdkPixbuf       *disabled_icon = NULL;

        nwamui_ncu_type_t       ncu_type;
        gboolean                enabled;
        
        
        g_return_val_if_fail( NWAMUI_IS_NCU( ncu ), NULL );
        
        ncu_type = nwamui_ncu_get_ncu_type(ncu);
        enabled = nwamui_ncu_get_active(ncu);

        if ( enabled_wired_icon == NULL ) { /* Load all icons */

            enabled_wired_icon = get_pixbuf( NWAM_ICON_NETWORK_IDLE, FALSE );

            
            enabled_wireless_icon = get_pixbuf( NWAM_ICON_NETWORK_IDLE, FALSE );
            
            
            disabled_icon = get_pixbuf( NWAM_ICON_NETWORK_OFFLINE, FALSE );
        }
        
        /* TODO - handle other status icon cases */
        if ( enabled ) {
            switch (ncu_type) {
                case NWAMUI_NCU_TYPE_WIRELESS:
                    return( GDK_PIXBUF(g_object_ref( G_OBJECT(enabled_wireless_icon) )) );
                case NWAMUI_NCU_TYPE_WIRED: 
                    /* Fall-through */
                default:
                    return( GDK_PIXBUF(g_object_ref( G_OBJECT(enabled_wired_icon) )) );
            }
        }
        else {
            return( GDK_PIXBUF(g_object_ref( G_OBJECT(disabled_icon) )) );
        }
}
       
extern GdkPixbuf*
nwamui_util_get_wireless_strength_icon( nwamui_wifi_signal_strength_t signal_strength, gboolean small )
{
    static GdkPixbuf*   enabled_wireless_icons[NWAMUI_WIFI_STRENGTH_LAST][2];
    gint                icon_size = small?0:1;
    
    if ( enabled_wireless_icons[NWAMUI_WIFI_STRENGTH_NONE][icon_size] == NULL ) {
        /* TODO - Get wireless icons to match the various strengths */
        enabled_wireless_icons[NWAMUI_WIFI_STRENGTH_NONE][icon_size] = 
                                    get_pixbuf( NWAM_ICON_WIRELESS_STRENGTH_NONE, small );

        enabled_wireless_icons[NWAMUI_WIFI_STRENGTH_VERY_WEAK][icon_size] = 
        enabled_wireless_icons[NWAMUI_WIFI_STRENGTH_WEAK][icon_size] = 
                                    g_object_ref(get_pixbuf( NWAM_ICON_WIRELESS_STRENGTH_POOR, small ));

        enabled_wireless_icons[NWAMUI_WIFI_STRENGTH_GOOD][icon_size] = 
                                    get_pixbuf( NWAM_ICON_WIRELESS_STRENGTH_FAIR, small );

        enabled_wireless_icons[NWAMUI_WIFI_STRENGTH_VERY_GOOD][icon_size]= 
                                    g_object_ref(get_pixbuf( NWAM_ICON_WIRELESS_STRENGTH_GOOD, small ));
        enabled_wireless_icons[NWAMUI_WIFI_STRENGTH_EXCELLENT][icon_size]= 
                                    g_object_ref(get_pixbuf( NWAM_ICON_WIRELESS_STRENGTH_EXCELLENT, small ));

    }
    
    return( GDK_PIXBUF(g_object_ref( G_OBJECT(enabled_wireless_icons[signal_strength][icon_size]) )) );
    
}

/* 
 * Shows Help Dialog - the link_name refers to either the anchor or sectionid in the help file.
 */
extern void
nwamui_util_show_help( gchar* link_name )
{
  GError *error = NULL;
  
  gnome_help_display("nwam-manager", link_name, &error);
  
  if (error) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_MODAL, 
                   GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                   "<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
                   _("Could not display help"),
                   error->message);
    g_signal_connect (G_OBJECT (dialog), "response",
          G_CALLBACK (gtk_widget_destroy),
          NULL);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    gtk_widget_show (dialog);

    g_error_free (error);
  }
}

/*
 * Shows a dialog with a single entry like:
 * 
 *      Name: [     ] 
 *
 *      [Cancel] [OK]
 *
 */
gchar* 
nwamui_util_rename_dialog_run(GtkWindow* parent_window, const gchar* title, const gchar* current_name) 
{
    static GtkWidget*   dialog = NULL;
    static GtkWidget*   entry  = NULL;
    
    gint                response;
    gchar*              outstr;

    g_assert( title != NULL && current_name != NULL );

    g_return_val_if_fail( (title != NULL && current_name != NULL), NULL );

    if ( dialog == NULL ) {
        dialog = nwamui_util_glade_get_widget(NWAM_ENVIRONMENT_RENAME);
        entry = nwamui_util_glade_get_widget(RENAME_ENVIRONMENT_ENTRY);
    }

    if (parent_window != NULL) {
        gtk_window_set_transient_for (GTK_WINDOW(dialog), parent_window);
        gtk_window_set_modal (GTK_WINDOW(dialog), TRUE);
    }
    else {
        gtk_window_set_transient_for (GTK_WINDOW(dialog), NULL);
        gtk_window_set_modal (GTK_WINDOW(dialog), FALSE);
    }

    gtk_window_set_title(GTK_WINDOW(dialog), title );
    gtk_entry_set_text(GTK_ENTRY(entry), current_name );

    gtk_widget_show_all(dialog);
    response = gtk_dialog_run( GTK_DIALOG(dialog) );

    switch( response ) {
       case GTK_RESPONSE_OK:
           outstr = g_strdup(gtk_entry_get_text( GTK_ENTRY(entry) ));
           gtk_widget_hide_all(dialog);
           return( outstr );
       default:
           gtk_widget_hide_all(dialog);
           return( NULL );
    }
}

extern gboolean
nwamui_util_ask_yes_no(GtkWindow* parent_window, const gchar* title, const gchar* message) 
{
    GtkWidget*          message_dialog;
    gint                response;
    gchar*              outstr;

    g_assert( message != NULL );

    g_return_val_if_fail( message != NULL, FALSE );

    message_dialog = gtk_message_dialog_new(parent_window, GTK_DIALOG_MODAL |GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, message );
    
    if ( title != NULL ) {
        gtk_window_set_title(GTK_WINDOW(message_dialog), title);
    }
    
    gtk_message_dialog_format_secondary_text( GTK_MESSAGE_DIALOG(message_dialog), _("This operation cannot be undone.") );
    
    switch( gtk_dialog_run(GTK_DIALOG(message_dialog)) ) {
        case GTK_RESPONSE_YES:
            gtk_widget_destroy(message_dialog);
            return( TRUE );
        default:
            gtk_widget_destroy(message_dialog);
            return( FALSE );
    }
}

extern void
nwamui_util_show_message(GtkWindow* parent_window, GtkMessageType type, const gchar* title, const gchar* message)
{
    GtkWidget*          message_dialog;
    gint                response;
    gchar*              outstr;

    g_assert( message != NULL );
    
    g_assert( type != GTK_MESSAGE_QUESTION ); /* Should use nwamui_util_ask_yes_no() for this type */

    g_return_if_fail( message != NULL );

    message_dialog = gtk_message_dialog_new(parent_window, GTK_DIALOG_MODAL |GTK_DIALOG_DESTROY_WITH_PARENT,
                                            type, GTK_BUTTONS_CLOSE, message );
    
    if ( title != NULL ) {
        gtk_window_set_title(GTK_WINDOW(message_dialog), title);
    }
    
    /* Ensure dialog is destroryed when user closes it */
    g_signal_connect_swapped (message_dialog, "response", G_CALLBACK (gtk_widget_destroy), message_dialog);
    
   (void)gtk_dialog_run(GTK_DIALOG(message_dialog));
}

extern GList*
nwamui_util_map_condition_strings_to_object_list( char** conditions )
{
    GList*  new_list = NULL;

    if ( conditions == NULL ) {
        return( NULL );
    }

    for ( int i = 0; conditions[i] != NULL; i++ ) {
        NwamuiCond* cond = nwamui_cond_new_from_str( conditions[i] );
        if ( cond != NULL ) {
            new_list = g_list_append( new_list, cond);
        }
    }

    return( new_list );
}

extern GList*
nwamui_util_strv_to_glist( gchar **strv ) 
{
    GList   *new_list = NULL;

    for ( char** strp = strv; strp != NULL && *strp != NULL; strp++ ) {
        new_list = g_list_append( new_list, g_strdup( *strp ) );
    }

    return ( new_list );
}

extern gchar**
nwamui_util_glist_to_strv( GList *list ) 
{
    gchar** new_strv = NULL;

    if ( list != NULL ) {
        int     list_len = g_list_length( list );
        int     i = 0;

        new_strv = (gchar**)g_malloc0( sizeof(gchar*) * (list_len+1) );

        i = 0;
        for ( GList *element  = g_list_first( list );
              element != NULL && element->data != NULL;
              element = g_list_next( element ) ) {
            new_strv[i]  = g_strdup ( element->data );
            i++;
        }
        new_strv[list_len]=NULL;
    }

    return ( new_strv );
}

/* If there is any underscores we need to replace them with two since
 * otherwise it's interpreted as a mnemonic
 *
 * Will modify the label, possibly reallocating memory.
 *
 * Returns the modified pointer.
 */
extern gchar*
nwamui_util_encode_menu_label( gchar **modified_label )
{
    if ( modified_label == NULL ) {
        return NULL;
    }

    if ( *modified_label != NULL && strchr( *modified_label, '_' ) != NULL ) {
        /* Allocate a GString, with space for 2 extra underscores to
         * miminize need to reallocate, several times, but using GString
         * provides to possibility that it may grow.
         */
        GString *gstr = g_string_sized_new( strlen(*modified_label + 2 ) );
        for ( gchar *c = *modified_label; c != NULL && *c != '\0'; c++ ) {
            if ( *c == '_' ) {
                /* add extra underscore */
                g_string_append_c( gstr, '_' );
            }
            g_string_append_c( gstr, *c );
        }
        g_free(*modified_label);
        *modified_label = g_string_free(gstr, FALSE);
    }

    return( *modified_label );
}

#define MAX_NOTIFICATION_FAILURES   (20)

extern gboolean
notification_area_ready ( GtkStatusIcon* status_icon )
{
    static gint count_attempts = 0;

    GdkAtom selection_atom;
    GdkDisplay *display;
    Display* xdisplay;
    Window *manager_window;
    gboolean retval = FALSE;
    int i, screen_count;
    char buffer[256];
    display = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY( display );

    /* Only allow this to fail a max number of times otherwise we will never
     * show messages */
    if ( (count_attempts++) > MAX_NOTIFICATION_FAILURES ) {
        g_debug("notification_area_ready() returning TRUE (max failues reached)");
        return TRUE;
    }

    screen_count = gdk_display_get_n_screens (display);

    gdk_x11_grab_server();

    for (i=0; i<screen_count; i++) {
        g_snprintf (buffer, sizeof (buffer),
                    "_NET_SYSTEM_TRAY_S%d", i);
                    
        selection_atom = gdk_atom_intern(buffer, FALSE);
  
        manager_window = XGetSelectionOwner( xdisplay, gdk_x11_atom_to_xatom(selection_atom) );

        if ( manager_window != NULL ) {
            break;
        }
  
    }

    gdk_x11_ungrab_server();

    gdk_flush();

    if (manager_window != NULL ) {
        /* We need to process all events to get status of embedding
         * correctly, otherwise is always FALSE */
        while (gtk_events_pending ())
          gtk_main_iteration ();

        if ( status_icon != NULL && gtk_status_icon_is_embedded( status_icon ) ) {
            retval = TRUE;
            /* Make it always return TRUE from now on */
            count_attempts = MAX_NOTIFICATION_FAILURES + 1;
        }
    }

    g_debug("notification_area_ready() returning %s (attempts = %d)", retval?"TRUE":"FALSE", count_attempts);
    
    return retval;
}

/* VOID:INT,POINTER */
void
marshal_VOID__INT_POINTER (GClosure     *closure,
  GValue       *return_value G_GNUC_UNUSED,
  guint         n_param_values,
  const GValue *param_values,
  gpointer      invocation_hint G_GNUC_UNUSED,
  gpointer      marshal_data)
{
    typedef void (*GMarshalFunc_VOID__INT_POINTER) (gpointer     data1,
      gint         arg_1,
      gpointer     arg_2,
      gpointer     data2);
    register GMarshalFunc_VOID__INT_POINTER callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1, data2;
 
    g_return_if_fail (n_param_values == 3);
 
    if (G_CCLOSURE_SWAP_DATA (closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer (param_values + 0);
    } else {
        data1 = g_value_peek_pointer (param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__INT_POINTER) (marshal_data ? marshal_data : cc->callback);
 
    callback (data1,
      g_value_get_int (param_values + 1),
      g_value_get_pointer (param_values + 2),
      data2);
}

/* VOID:OBJECT, OBJECT */
void
marshal_VOID__OBJECT_OBJECT (GClosure     *closure,
  GValue       *return_value G_GNUC_UNUSED,
  guint         n_param_values,
  const GValue *param_values,
  gpointer      invocation_hint G_GNUC_UNUSED,
  gpointer      marshal_data)
{
    typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT) (gpointer     data1,
      gpointer     arg_1,
      gpointer     arg_2,
      gpointer     data2);
    register GMarshalFunc_VOID__OBJECT_OBJECT callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1, data2;
 
    g_return_if_fail (n_param_values == 3);
 
    if (G_CCLOSURE_SWAP_DATA (closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer (param_values + 0);
    } else {
        data1 = g_value_peek_pointer (param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);
 
    callback (data1,
      g_value_get_object (param_values + 1),
      g_value_get_object (param_values + 2),
      data2);
}

/* VOID:INT, OBJECT, POINTER */
void
marshal_VOID__INT_OBJECT_POINTER (GClosure     *closure,
  GValue       *return_value G_GNUC_UNUSED,
  guint         n_param_values,
  const GValue *param_values,
  gpointer      invocation_hint G_GNUC_UNUSED,
  gpointer      marshal_data)
{
    typedef void (*GMarshalFunc_VOID__INT_OBJECT_POINTER) (gpointer     data1,
      gint         arg_1,
      gpointer     arg_2,
      gpointer     arg_3,
      gpointer     data2);
    register GMarshalFunc_VOID__INT_OBJECT_POINTER callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1, data2;
 
    g_return_if_fail (n_param_values == 4);
 
    if (G_CCLOSURE_SWAP_DATA (closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer (param_values + 0);
    } else {
        data1 = g_value_peek_pointer (param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_VOID__INT_OBJECT_POINTER) (marshal_data ? marshal_data : cc->callback);
 
    callback (data1,
      g_value_get_int (param_values + 1),
      g_value_get_object (param_values + 2),
      g_value_get_pointer (param_values + 3),
      data2);
}

/* OBJECT:VOID */
void
marshal_OBJECT__VOID(GClosure     *closure,
  GValue       *return_value,
  guint         n_param_values,
  const GValue *param_values,
  gpointer      invocation_hint G_GNUC_UNUSED,
  gpointer      marshal_data)
{
    typedef GObject* (*GMarshalFunc_OBJECT__VOID) (gpointer     data1,
      gpointer     data2);
    register GMarshalFunc_OBJECT__VOID callback;
    register GCClosure *cc = (GCClosure*) closure;
    register gpointer data1, data2;
    register GObject* obj;
 
    g_return_if_fail (n_param_values == 1);
 
    if (G_CCLOSURE_SWAP_DATA (closure)) {
        data1 = closure->data;
        data2 = g_value_peek_pointer (param_values + 0);
    } else {
        data1 = g_value_peek_pointer (param_values + 0);
        data2 = closure->data;
    }
    callback = (GMarshalFunc_OBJECT__VOID) (marshal_data ? marshal_data : cc->callback);
 
    obj = callback (data1,
      data2);

    g_value_take_object(return_value, obj);
}
