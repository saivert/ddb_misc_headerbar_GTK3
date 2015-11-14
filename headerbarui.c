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
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#if ENABLE_NLS

# include <libintl.h>
#define _(s) gettext(s)

#else

#define _(s) (s)

#endif

#define trace(...) { fprintf(stderr, __VA_ARGS__); }

DB_functions_t *deadbeef;
static DB_misc_t plugin;
static ddb_gtkui_t *gtkui_plugin;
static const char settings_dlg[];
GtkWidget *headerbar;
GtkWidget *volbutton;
GtkMenu *headerbarui_menu;
GtkWidget *headerbar_seekbar;
guint headerbar_timer;

gboolean seekbar_ismoving=FALSE;

static gboolean
headerbarui_action_gtk (void *data)
{
    return FALSE;
}

static int
headerbarui_action_callback(DB_plugin_action_t *action, int ctx) {
    headerbarui_action_gtk(NULL);
    //g_idle_add (headerbarui_action_gtk, NULL);
    return 0;
}

static DB_plugin_action_t headerbarui_action = {
    .title = "Edit/Configure Headerbar",
    .name = "headerbar_conf",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = headerbarui_action_callback,
    .next = NULL,
};

static DB_plugin_action_t *
headerbarui_getactions(DB_playItem_t *it) {
    return &headerbarui_action;
}

GtkWidget*
lookup_widget                          (GtkWidget       *widget,
                                        const gchar     *widget_name)
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

#if 0
gboolean
seek_cb (gpointer data)
{
    int *value = data;
    deadbeef->sendmessage (DB_EV_SEEK, 0, (*value) * 1000, 0);
    return 0;
}
#endif

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
    return TRUE;
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

void
gtkui_create_playback_controls_in_headerbar(GtkWidget* headerbar)
{
    GtkWidget *stopbtn;
    GtkWidget *image128;
    GtkWidget *playbtn;
    GtkWidget *image2;
    GtkWidget *pausebtn;
    GtkWidget *image3;
    GtkWidget *prevbtn;
    GtkWidget *image4;
    GtkWidget *nextbtn;
    GtkWidget *image5;

    nextbtn = gtk_button_new ();
    gtk_widget_show (nextbtn);
    gtk_header_bar_pack_end(GTK_HEADER_BAR (headerbar), nextbtn);
    gtk_widget_set_can_focus(nextbtn, FALSE);

    image5 = gtk_image_new_from_icon_name ("media-skip-forward-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image5);
    gtk_container_add (GTK_CONTAINER (nextbtn), image5);

    prevbtn = gtk_button_new ();
    gtk_widget_show (prevbtn);
    gtk_header_bar_pack_end(GTK_HEADER_BAR (headerbar), prevbtn);
    gtk_widget_set_can_focus(prevbtn, FALSE);

    image4 = gtk_image_new_from_icon_name ("media-skip-backward-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image4);
    gtk_container_add (GTK_CONTAINER (prevbtn), image4);

    pausebtn = gtk_button_new ();
    gtk_widget_show (pausebtn);
    gtk_header_bar_pack_end(GTK_HEADER_BAR (headerbar), pausebtn);
    gtk_widget_set_can_focus(pausebtn, FALSE);

    image3 = gtk_image_new_from_icon_name ("media-playback-pause-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image3);
    gtk_container_add (GTK_CONTAINER (pausebtn), image3);

    playbtn = gtk_button_new ();
    gtk_widget_show (playbtn);
    gtk_header_bar_pack_end(GTK_HEADER_BAR (headerbar), playbtn);
    gtk_widget_set_can_focus(playbtn, FALSE);

    image2 = gtk_image_new_from_icon_name ("media-playback-start-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image2);
    gtk_container_add (GTK_CONTAINER (playbtn), image2);

    stopbtn = gtk_button_new ();
    gtk_widget_show (stopbtn);
    gtk_header_bar_pack_end(GTK_HEADER_BAR (headerbar), stopbtn);
    gtk_widget_set_can_focus(stopbtn, FALSE);

    image128 = gtk_image_new_from_icon_name ("media-playback-stop-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image128);
    gtk_container_add (GTK_CONTAINER (stopbtn), image128);

    g_signal_connect ((gpointer) stopbtn, "clicked",
            G_CALLBACK (on_stopbtn_clicked),
            NULL);
    g_signal_connect ((gpointer) playbtn, "clicked",
            G_CALLBACK (on_playbtn_clicked),
            NULL);
    g_signal_connect ((gpointer) pausebtn, "clicked",
            G_CALLBACK (on_pausebtn_clicked),
            NULL);
    g_signal_connect ((gpointer) prevbtn, "clicked",
            G_CALLBACK (on_prevbtn_clicked),
            NULL);
    g_signal_connect ((gpointer) nextbtn, "clicked",
            G_CALLBACK (on_nextbtn_clicked),
            NULL);
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
    }
    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
    return TRUE;
}

gboolean
headerbarui_reset_cb(gpointer user_data)
{
    gtk_range_set_value(GTK_RANGE (headerbar_seekbar), 0);
    return FALSE;
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

static gboolean
headerbarui_init () {
    GtkWidget *mainwin = gtkui_plugin->get_mainwin ();
    GtkWidget *menubar = lookup_widget (mainwin, "menubar");
    GtkWidget *menubtn;
    GtkWidget *menubtn_image;
    int embed_menubar = deadbeef->conf_get_int ("headerbarui.embed_menubar", 0);
    int show_seek_bar = deadbeef->conf_get_int ("headerbarui.show_seek_bar", 1);

    headerbar = gtk_header_bar_new();
    volbutton = gtk_volume_button_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR (headerbar), "DeaDBeeF");
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR (headerbar), TRUE);
    gtk_header_bar_pack_end(GTK_HEADER_BAR (headerbar), GTK_WIDGET (volbutton));

    if (!embed_menubar)
    {
        gtk_widget_hide(menubar);

        headerbarui_menu = GTK_MENU (gtk_menu_new ());
        GList *l;
        for (l = gtk_container_get_children(GTK_CONTAINER (menubar)); l != NULL; l = l->next)
        {
            gtk_widget_reparent(GTK_WIDGET (l->data), GTK_WIDGET (headerbarui_menu));
        }
        menubtn = gtk_menu_button_new ();
        gtk_menu_button_set_popup(GTK_MENU_BUTTON (menubtn), GTK_WIDGET(headerbarui_menu));
        gtk_widget_show (menubtn);
        gtk_header_bar_pack_start(GTK_HEADER_BAR (headerbar), menubtn);
        gtk_widget_set_can_focus(menubtn, FALSE);

        menubtn_image = gtk_image_new_from_icon_name ("open-menu-symbolic", GTK_ICON_SIZE_MENU);
        gtk_widget_show (menubtn_image);
        gtk_container_add (GTK_CONTAINER (menubtn), menubtn_image);        
    } else {
        gtk_widget_reparent(menubar, headerbar);
    }

    gtkui_create_playback_controls_in_headerbar(headerbar);

    if (show_seek_bar)
    {
        headerbar_seekbar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);
        gtk_widget_set_size_request(headerbar_seekbar, 200, -1);
        gtk_widget_set_hexpand(headerbar_seekbar, TRUE);
        gtk_scale_set_value_pos(GTK_SCALE (headerbar_seekbar), GTK_POS_RIGHT);
        gtk_header_bar_set_custom_title(GTK_HEADER_BAR (headerbar), headerbar_seekbar);
        gtk_widget_show(headerbar_seekbar);
    }

    int curvol=-(deadbeef->volume_get_min_db()-deadbeef->volume_get_db());
    gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON (volbutton), gtk_adjustment_new (-curvol, 0, (int)-deadbeef->volume_get_min_db (), 5, 5, 0));
    gtk_widget_show(volbutton);

    g_signal_connect ((gpointer) headerbar_seekbar, "button_release_event",
                     G_CALLBACK (on_seekbar_button_release_event),
                     NULL);
    g_signal_connect ((gpointer) headerbar_seekbar, "button_press_event",
                     G_CALLBACK (on_seekbar_button_press_event),
                     NULL);
    g_signal_connect ((gpointer) headerbar_seekbar, "format-value",
                    G_CALLBACK (on_seekbar_format_value),
                    NULL);
    g_signal_connect ((gpointer) volbutton, "value-changed",
                    G_CALLBACK (on_volbutton_value_changed),
                    NULL);

    gtk_widget_show(headerbar);

    gtk_window_set_titlebar(GTK_WINDOW (mainwin), GTK_WIDGET(headerbar));

    return FALSE;
}

int headerbarui_connect() {
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
gtkui_volume_changed(gpointer user_data)
{
    float volume = deadbeef->volume_get_min_db()-deadbeef->volume_get_db();

    gtk_scale_button_set_value( GTK_SCALE_BUTTON (volbutton), (int)-volume );
    return 0;
}


static int
headerbarui_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_SONGSTARTED:
        headerbar_timer = g_timeout_add (1000/gtkui_get_gui_refresh_rate (), headerbarui_update_seekbar_cb, NULL);
        break;
    case DB_EV_SONGFINISHED:
        g_source_remove(headerbar_timer);
        g_idle_add(headerbarui_reset_cb, NULL);
        break;
    case DB_EV_CONFIGCHANGED:
    case DB_EV_VOLUMECHANGED:
        g_idle_add (gtkui_volume_changed, NULL);
        break;
    }
    return 0;
}

static const char settings_dlg[] =
    "property \"Show seekbar\" checkbox headerbarui.show_seek_bar 1;\n"
    "property \"Embed menubar instead of showing hamburger button\" checkbox headerbarui.embed_menubar 1;\n"
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
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
