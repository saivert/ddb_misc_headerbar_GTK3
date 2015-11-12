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

GtkWidget *headerbar;
GtkWidget *volbutton;
GtkMenu *headerbarui_menu;

static gboolean
headerbarui_action_gtk (void *data)
{
    GtkWidget *mainwin = gtkui_plugin->get_mainwin ();
    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Not implemented."));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    gtk_window_set_title (GTK_WINDOW (dlg), _("Error"));

    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);

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

static gint
on_menubtn_clicked (GtkButton *button, GdkEvent *event)
{
    GdkEventButton *event_button;

    g_return_val_if_fail (headerbarui_menu != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_MENU (headerbarui_menu), FALSE);

    if (event->type == GDK_BUTTON_PRESS)
    {
      event_button = (GdkEventButton *) event;
      if (event_button->button == GDK_BUTTON_PRIMARY)
      {
          gtk_menu_popup (headerbarui_menu, NULL, NULL, NULL, NULL, 
              event_button->button, event_button->time);
          return TRUE;
      }
  }

  return FALSE;
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
    GtkWidget *menubtn;
    GtkWidget *image6;

    nextbtn = gtk_button_new ();
    gtk_widget_show (nextbtn);
    gtk_header_bar_pack_end(headerbar, nextbtn);
    gtk_widget_set_can_focus(nextbtn, FALSE);
    // gtk_button_set_relief (GTK_BUTTON (nextbtn), GTK_RELIEF_NONE);

    image5 = gtk_image_new_from_icon_name ("media-skip-forward-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image5);
    gtk_container_add (GTK_CONTAINER (nextbtn), image5);

    prevbtn = gtk_button_new ();
    gtk_widget_show (prevbtn);
    gtk_header_bar_pack_end(headerbar, prevbtn);
    gtk_widget_set_can_focus(prevbtn, FALSE);
    // gtk_button_set_relief (GTK_BUTTON (prevbtn), GTK_RELIEF_NONE);

    image4 = gtk_image_new_from_icon_name ("media-skip-backward-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image4);
    gtk_container_add (GTK_CONTAINER (prevbtn), image4);

    pausebtn = gtk_button_new ();
    gtk_widget_show (pausebtn);
    gtk_header_bar_pack_end(headerbar, pausebtn);
    gtk_widget_set_can_focus(pausebtn, FALSE);
    // gtk_button_set_relief (GTK_BUTTON (pausebtn), GTK_RELIEF_NONE);

    image3 = gtk_image_new_from_icon_name ("media-playback-pause-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image3);
    gtk_container_add (GTK_CONTAINER (pausebtn), image3);

    playbtn = gtk_button_new ();
    gtk_widget_show (playbtn);
    gtk_header_bar_pack_end(headerbar, playbtn);
    gtk_widget_set_can_focus(playbtn, FALSE);
    // gtk_button_set_relief (GTK_BUTTON (playbtn), GTK_RELIEF_NONE);

    image2 = gtk_image_new_from_icon_name ("media-playback-start-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image2);
    gtk_container_add (GTK_CONTAINER (playbtn), image2);

    stopbtn = gtk_button_new ();
    gtk_widget_show (stopbtn);
    gtk_header_bar_pack_end(headerbar, stopbtn);
    gtk_widget_set_can_focus(stopbtn, FALSE);
    // gtk_button_set_relief (GTK_BUTTON (stopbtn), GTK_RELIEF_NONE);

    image128 = gtk_image_new_from_icon_name ("media-playback-stop-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image128);
    gtk_container_add (GTK_CONTAINER (stopbtn), image128);

    menubtn = gtk_menu_button_new ();
    gtk_menu_button_set_popup(menubtn, headerbarui_menu);
    gtk_widget_show (menubtn);
    gtk_header_bar_pack_start(headerbar, menubtn);
    gtk_widget_set_can_focus(menubtn, FALSE);
    // gtk_button_set_relief (GTK_BUTTON (menubtn), GTK_RELIEF_NONE);

    image6 = gtk_image_new_from_icon_name ("open-menu-symbolic", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image6);
    gtk_container_add (GTK_CONTAINER (menubtn), image6);

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
    // g_signal_connect ((gpointer) menubtn, "button_press_event",
    //         G_CALLBACK (on_menubtn_clicked),
    //         NULL);

}

static gboolean
headerbarui_init () {
    GtkWidget *mainwin = gtkui_plugin->get_mainwin ();
    GtkWidget *menubar = lookup_widget (mainwin, "menubar");

    gtk_widget_hide(menubar);

    headerbarui_menu = gtk_menu_new ();
    GList *l;
    for (l = gtk_container_get_children(menubar); l != NULL; l = l->next)
    {
        gtk_widget_reparent(GTK_MENU_ITEM (l->data), headerbarui_menu);
    }

    headerbar = gtk_header_bar_new();
    volbutton = gtk_volume_button_new();
    gtk_header_bar_set_title(headerbar, "DeaDBeeF");
    gtk_header_bar_set_show_close_button(headerbar, TRUE);
    gtk_header_bar_pack_end(headerbar, volbutton);

    gtkui_create_playback_controls_in_headerbar(headerbar);

    int curvol=-(deadbeef->volume_get_min_db()-deadbeef->volume_get_db());
    gtk_scale_button_set_adjustment(volbutton, gtk_adjustment_new (curvol, 0, (int)-deadbeef->volume_get_min_db (), 5, 5, 0));
    gtk_widget_show(volbutton);

    g_signal_connect ((gpointer) volbutton, "value-changed",
                    G_CALLBACK (on_volbutton_value_changed),
                    NULL);

    gtk_widget_show(headerbar);

    gtk_window_set_titlebar(mainwin, headerbar);

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

void
gtkui_volume_changed(void)
{
    float volume = deadbeef->volume_get_min_db()-deadbeef->volume_get_db();

    gtk_scale_button_set_value( volbutton, (int)-volume );
}


static int
headerbarui_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
    case DB_EV_VOLUMECHANGED:
        g_idle_add (gtkui_volume_changed, NULL);
        break;
    }
    return 0;
}


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
    .plugin.get_actions = headerbarui_getactions,
    .plugin.connect = headerbarui_connect,
    .plugin.message = headerbarui_message,
};

DB_plugin_t *
ddb_misc_headerbar_GTK3_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
