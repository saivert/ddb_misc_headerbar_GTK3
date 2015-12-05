/*
    Headerbar UI plugin for the DeaDBeeF audio player

    Copyright (C) 2015 Nicolai Syvertsen <saivert@gmail.com>
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include "appmenu.h"
#include "resources.h"
#include "headerbarui.h"

DB_functions_t *deadbeef;

static ddb_gtkui_t *gtkui_plugin;
static gint mainwin_width;

GtkWidget *headerbar;
GtkWidget *volbutton;
GtkWidget *headerbar_seekbar;
GtkWidget *headerbar_playbtn;
GtkWidget *headerbar_pausebtn;
GtkWidget *headerbar_menubtn;

guint headerbar_timer;

gboolean seekbar_ismoving = FALSE;

static struct headerbarui_flag_s {
    gboolean embed_menubar;
    gboolean show_seek_bar;
    gboolean hide_seekbar_on_streaming;
} headerbarui_flags;

GtkWidget*
lookup_widget (GtkWidget *widget, const gchar *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = gtk_widget_get_parent (widget);
      if (!parent)
        parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),
                                                 widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);
  return found_widget;
}

void
on_volbutton_value_changed (GtkScaleButton *button,
               gdouble         value,
               gpointer        user_data)
{
        deadbeef->volume_set_db (deadbeef->volume_get_min_db()-(double)-value);
}

gboolean
on_seekbar_button_press_event (GtkScale *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
    seekbar_ismoving = TRUE;
    return FALSE;
}

gboolean
on_seekbar_button_release_event (GtkScale *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (trk) {
        if (deadbeef->pl_get_item_duration (trk) >= 0) {
            int value = (int)gtk_range_get_value(GTK_RANGE (widget));
            deadbeef->sendmessage (DB_EV_SEEK, 0, (int)value * 1000, 0);
        }
        deadbeef->pl_item_unref (trk);
    }    

    seekbar_ismoving = FALSE;
    return FALSE;
}

static gchar*
on_seekbar_format_value (GtkScale *scale,
                gdouble value)
{
    int time = value;
    int hr = time/3600;
    int mn = (time-hr*3600)/60;
    int sc = time-hr*3600-mn*60;
    return g_strdup_printf ("%02d:%02d:%02d", hr, mn, sc);
}

void
on_stopbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_STOP, 0, 0, 0);
}


void
on_playbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    // NOTE: this function is a copy of action_play_cb
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == OUTPUT_STATE_PAUSED) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
        if (cur != -1) {
            ddb_playItem_t *it = deadbeef->plt_get_item_for_idx (plt, cur, PL_MAIN);
            ddb_playItem_t *it_playing = deadbeef->streamer_get_playing_track ();

            if (it) {
                deadbeef->pl_item_unref (it);
            }
            if (it_playing) {
                deadbeef->pl_item_unref (it_playing);
            }
            if (it != it_playing) {
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, cur, 0);
            }
            else {
                deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
            }
        }
        else {
            deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
        }
        deadbeef->plt_unref (plt);
    }
    else {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cur = -1;
        if (plt) {
            cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
            deadbeef->plt_unref (plt);
        }
        if (cur == -1) {
            cur = 0;
        }
        deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, cur, 0);
    }
}


void
on_pausebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
}


void
on_prevbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
}


void
on_nextbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
}

gboolean
headerbarui_reset_cb(gpointer user_data)
{
    GtkAdjustment * adjustment = gtk_range_get_adjustment(GTK_RANGE (headerbar_seekbar));
    gtk_adjustment_configure(adjustment,
    deadbeef->streamer_get_playpos (), //value
    0, // lower
    0, // upper
    0, // step_increment
    0, // page_increment
    0); // page_size
    gtk_range_set_value(GTK_RANGE (headerbar_seekbar), 0);
    gtk_scale_set_draw_value(GTK_SCALE(headerbar_seekbar), FALSE);
    return FALSE;
}

gboolean
headerbarui_update_seekbar_cb(gpointer user_data)
{
    if (seekbar_ismoving) return TRUE;
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (!trk || deadbeef->pl_get_item_duration (trk) < 0) {
        if (trk) {
            deadbeef->pl_item_unref (trk);
        }
        if (headerbarui_flags.hide_seekbar_on_streaming)
            gtk_widget_hide(headerbar_seekbar);
        else
            headerbarui_reset_cb(NULL);
        return TRUE;
    }
    if (deadbeef->pl_get_item_duration (trk) > 0) {
        GtkAdjustment * adjustment = gtk_range_get_adjustment(GTK_RANGE (headerbar_seekbar));
        gtk_adjustment_configure(adjustment,
            deadbeef->streamer_get_playpos (), //value
            0, // lower
            deadbeef->pl_get_item_duration (trk), // upper
            1, // step_increment
            1, // page_increment
            0); // page_size
        gtk_scale_set_draw_value(GTK_SCALE(headerbar_seekbar), TRUE);
        if (headerbarui_flags.hide_seekbar_on_streaming)
            gtk_widget_show(headerbar_seekbar);
    }
    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
    return TRUE;
}



int
gtkui_get_gui_refresh_rate () {
    int fps = deadbeef->conf_get_int ("gtkui.refresh_rate", 10);
    if (fps < 1) {
        fps = 1;
    }
    else if (fps > 30) {
        fps = 30;
    }
    return fps;
}

GObject *
g_object_clone(GObject *src)
{
    GObject *dst;
    GParameter *params;
    GParamSpec **specs;
    guint n, n_specs, n_params;

    specs = g_object_class_list_properties(G_OBJECT_GET_CLASS(src), &n_specs);
    params = g_new0(GParameter, n_specs);
    n_params = 0;

    for (n = 0; n < n_specs; ++n)
        if (strcmp(specs[n]->name, "parent") &&
            (specs[n]->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE) {
            params[n_params].name = g_intern_string(specs[n]->name);
            g_value_init(&params[n_params].value, specs[n]->value_type);
            g_object_get_property(src, specs[n]->name, &params[n_params].value);
            ++ n_params;
        }

    dst = g_object_newv(G_TYPE_FROM_INSTANCE(src), n_params, params);
    g_free(specs);
    g_free(params);

    return dst;
}

void
headerbarui_update_menubutton()
{
    GtkWindow *mainwin;
    GtkWidget *menubar;
    static GtkMenu *menu;

    mainwin = GTK_WINDOW (gtkui_plugin->get_mainwin ());
    menubar = lookup_widget (GTK_WIDGET(mainwin), "menubar");

    menu = GTK_MENU (gtk_menu_new ());

    GList *l, *children;
    children = gtk_container_get_children(GTK_CONTAINER (menubar));
    for (l = children; l; l = l->next)
    {
        gtk_container_add(GTK_CONTAINER(menu), g_object_clone(l->data));
    }
    g_list_free(children);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON (headerbar_menubtn), GTK_WIDGET(menu));
}

static gint
seekbar_width () {
    return MIN((mainwin_width / 2) - 150, 420);
}

static gboolean
mainwindow_resize (GtkWindow *mainwindow,
                   GdkEventConfigure *event,
                   gpointer pointer) {
    if (headerbarui_flags.show_seek_bar && event->width != mainwin_width) {
        mainwin_width = event->width;
        gtk_widget_set_size_request (headerbar_seekbar,
            seekbar_width (),
            -1);
    }
    return FALSE;
}

static gboolean
headerbarui_init () {
    GtkWindow *mainwin;
    GtkWidget *menubar;
    GtkWidget *menubtn_image;
    GtkBuilder *builder;

    mainwin = GTK_WINDOW (gtkui_plugin->get_mainwin ());

    menubar = lookup_widget (GTK_WIDGET(mainwin), "menubar");
    g_assert_nonnull(mainwin);
    g_assert_nonnull(menubar);

    builder = gtk_builder_new_from_resource("/org/deadbeef/headerbarui/headerbar.ui");
    headerbar = gtk_builder_get_object(builder, "headerbar1");
    volbutton = gtk_builder_get_object(builder, "volumebutton1");
    headerbar_menubtn =  gtk_builder_get_object(builder, "menubutton1");
    headerbar_seekbar = gtk_builder_get_object(builder, "scale1");
    headerbar_playbtn = gtk_builder_get_object(builder, "playbtn");
    headerbar_pausebtn = gtk_builder_get_object(builder, "pausebtn");

    gtk_widget_show(headerbar);

    gtk_window_set_titlebar(mainwin, GTK_WIDGET(headerbar));

    if (!headerbarui_flags.embed_menubar)
    {
        GtkMenu *menu;
        gtk_widget_hide(menubar);

        headerbarui_update_menubutton();

        gtk_widget_set_can_focus(headerbar_menubtn, FALSE);
        gtk_widget_show (headerbar_menubtn);
    } else {
        gtk_widget_destroy(headerbar_menubtn);
        gtk_widget_reparent(menubar, headerbar);
    }

    if (!headerbarui_flags.show_seek_bar)
    {
        gtk_widget_hide(headerbar_seekbar);
    }

    float volume = deadbeef->volume_get_min_db()-deadbeef->volume_get_db();
    g_assert_false((volume>0));
    gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON (volbutton),
        gtk_adjustment_new (volume, 0, (int)-deadbeef->volume_get_min_db (), 5, 5, 0));

    gtk_widget_show(volbutton);

    gtk_builder_add_callback_symbols(builder,
        "on_volbutton_value_changed", on_volbutton_value_changed,
        "on_nextbtn_clicked", on_nextbtn_clicked,
        "on_prevbtn_clicked", on_prevbtn_clicked,
        "on_pausebtn_clicked", on_pausebtn_clicked,
        "on_playbtn_clicked", on_playbtn_clicked,
        "on_stopbtn_clicked", on_stopbtn_clicked,
        "on_seekbar_format_value", on_seekbar_format_value,
        "on_seekbar_button_press_event", on_seekbar_button_press_event,
        "on_seekbar_button_release_event", on_seekbar_button_release_event,
        NULL);
    gtk_builder_connect_signals(builder, NULL);

    gtk_window_get_size (mainwin, &mainwin_width, NULL);
    gtk_widget_set_size_request (headerbar_seekbar, seekbar_width (), -1);
    g_signal_connect (G_OBJECT(mainwin),
        "configure-event",
        G_CALLBACK(mainwindow_resize),
        NULL);

    return FALSE;
}

void headerbarui_getconfig()
{
    headerbarui_flags.embed_menubar = deadbeef->conf_get_int ("headerbarui.embed_menubar", 0);
    headerbarui_flags.show_seek_bar = deadbeef->conf_get_int ("headerbarui.show_seek_bar", 1);
    if (headerbarui_flags.show_seek_bar)
        headerbarui_flags.hide_seekbar_on_streaming = deadbeef->conf_get_int ("headerbarui.hide_seekbar_on_streaming", 0);
    else
        headerbarui_flags.hide_seekbar_on_streaming = FALSE;
}

int headerbarui_connect() {
    headerbarui_getconfig();
    gtkui_plugin = (ddb_gtkui_t *) deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    if (gtkui_plugin) {
        if (gtkui_plugin->gui.plugin.version_major == 2) {
            g_idle_add (headerbarui_init, NULL);
            return 0;
        }
    }
    return -1;
}

gboolean
headerbarui_volume_changed(gpointer user_data)
{
    float volume = deadbeef->volume_get_min_db()-deadbeef->volume_get_db();
    g_assert_false((volume>0));
    gtk_scale_button_set_value( GTK_SCALE_BUTTON (volbutton), (int)-volume );
    return FALSE;
}


gboolean
playpause_update(gpointer user_data)
{
    if (user_data)
        {
            gtk_widget_hide(headerbar_playbtn);
            gtk_widget_show(headerbar_pausebtn);
        }
    else
        {
            gtk_widget_show(headerbar_playbtn);
            gtk_widget_hide(headerbar_pausebtn);
        }
    return FALSE;
}

static int
headerbarui_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_SONGSTARTED:
        headerbar_timer = g_timeout_add (1000/gtkui_get_gui_refresh_rate (), headerbarui_update_seekbar_cb, NULL);
        g_idle_add(playpause_update, TRUE);
        break;
    case DB_EV_SONGFINISHED:
        g_source_remove(headerbar_timer);
        g_idle_add(headerbarui_reset_cb, NULL);
        g_idle_add(playpause_update, FALSE);
        break;
    case DB_EV_PAUSED:
        g_idle_add(playpause_update, !p1);
        break;
    case DB_EV_CONFIGCHANGED:
        headerbarui_getconfig();
        g_idle_add (headerbarui_volume_changed, NULL);
        break;
    case DB_EV_VOLUMECHANGED:
        g_idle_add (headerbarui_volume_changed, NULL);
        break;
    }
    return 0;
}

static const char settings_dlg[] =
    "property \"Show seekbar\" checkbox headerbarui.show_seek_bar 1;\n"
    "property \"Embed menubar instead of showing hamburger button\" checkbox headerbarui.embed_menubar 0;\n"
    "property \"Hide seekbar on streaming\" checkbox headerbarui.hide_seekbar_on_streaming 0;\n"
;

static DB_misc_t plugin = {
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 5,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.id = "headerbarui_gtk3",
    .plugin.name = "Headerbar for GTK3 UI",
    .plugin.descr = "A headerbar for the GTK3 UI",
    .plugin.copyright = 
        "Headerbar for GTK3 UI plugin for DeaDBeeF Player\n"
        "Copyright (C) 2015 Nicolai Syvertsen <saivert@gmail.com>\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n",
    .plugin.website = "http://saivert.com",
    //.plugin.get_actions = headerbarui_getactions,
    .plugin.configdialog = settings_dlg,
    .plugin.connect = headerbarui_connect,
    .plugin.message = headerbarui_message,
};

DB_plugin_t *
ddb_misc_headerbar_GTK3_load (DB_functions_t *api) {
    g_resources_register(headerbarui_get_resource());
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
