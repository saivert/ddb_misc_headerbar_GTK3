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
#include "resources.h"
#include "headerbarui.h"

DB_functions_t *deadbeef;
static DB_misc_t plugin;

static ddb_gtkui_t *gtkui_plugin;
static gint mainwin_width;

GtkWidget *headerbar;
GtkWidget *volbutton;
GtkWidget *headerbar_seekbar;
GtkWidget *headerbar_playbtn;
GtkWidget *headerbar_pausebtn;
GtkWidget *headerbar_stopbtn;
GtkWidget *headerbar_menubtn;
GtkWidget *headerbar_prefsbtn;
GtkWidget *headerbar_designmodebtn;

#define GTK_BUILDER_GET_WIDGET(builder, name) (GtkWidget *)gtk_builder_get_object(builder, name)

guint headerbar_timer;

gboolean seekbar_ismoving = FALSE;
gboolean seekbar_isvisible = FALSE;
gboolean headerbar_stoptimer = FALSE;

static struct headerbarui_flag_s {
    gboolean disable;
    gboolean embed_menubar;
    gboolean show_seek_bar;
    gboolean seekbar_minimized;
    gboolean hide_seekbar_on_streaming;
    gboolean combined_playpause;
    gboolean show_stop_button;
    gboolean show_volume_button;
    gboolean show_preferences_button;
    gboolean show_designmode_button;
    int button_spacing;
} headerbarui_flags;

static
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

static
void
on_volbutton_value_changed (GtkScaleButton *button,
               gdouble         value,
               gpointer        user_data)
{
        deadbeef->volume_set_db (deadbeef->volume_get_min_db()-(double)-value);
}


static
void
deadbeef_seek(int value)
{
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (trk) {
        if (deadbeef->pl_get_item_duration (trk) >= 0) {
            deadbeef->sendmessage (DB_EV_SEEK, 0, (int)value * 1000, 0);
        }
        deadbeef->pl_item_unref (trk);
    }
}

static
void
on_seekbar_value_changed (GtkRange *range,
               gpointer  user_data)
{
    if (seekbar_ismoving) return;
    deadbeef_seek((int)gtk_range_get_value(range));
}

static
gboolean
on_seekbar_button_press_event (GtkScale *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
    seekbar_ismoving = TRUE;
    return FALSE;
}

static
gboolean
on_seekbar_button_release_event (GtkScale *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
    deadbeef_seek((int)gtk_range_get_value(GTK_RANGE(widget)));
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
    if (hr==0)
        return g_strdup_printf ("%02d:%02d", mn, sc);
    else
        return g_strdup_printf ("%02d:%02d:%02d", hr, mn, sc);
}

static
void
headerbarui_adjust_range(GtkRange *range,
                          gdouble value,
                          gdouble lower,
                          gdouble upper,
                          gdouble step_increment,
                          gdouble page_increment,
                          gdouble page_size)
{
    GtkAdjustment * adjustment = gtk_range_get_adjustment(range);

    GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA);
    GQuark detail = g_quark_from_static_string("value_changed");
    g_signal_handlers_block_matched ((gpointer)range, mask, detail, 0, NULL, NULL, NULL);

    gtk_adjustment_configure(adjustment,
    value, //value
    lower, // lower
    upper, // upper
    step_increment, // step_increment
    page_increment, // page_increment
    page_size); // page_size

    g_signal_handlers_unblock_matched ((gpointer)range, mask, detail, 0, NULL, NULL, NULL);
}

static
gboolean
headerbarui_reset_seekbar_cb(gpointer user_data)
{
    if (!headerbarui_flags.show_seek_bar) return FALSE;

    headerbarui_adjust_range(GTK_RANGE (headerbar_seekbar),
        0, //value
        0, // lower
        0, // upper
        0, // step_increment
        0, // page_increment
        0); // page_size

    gtk_scale_set_draw_value(GTK_SCALE(headerbar_seekbar), FALSE);
    return FALSE;
}

void
playpause_update(int state);

static
gboolean
headerbarui_update_seekbar_cb(gpointer user_data)
{
    DB_playItem_t *trk;
    DB_output_t *out;
    seekbar_isvisible = TRUE;

    out = deadbeef->get_output();
    if (out) {
        playpause_update(out->state());
        if (out->state() == OUTPUT_STATE_STOPPED) {
            seekbar_isvisible = FALSE;
            goto END;
        }
    }

    if (seekbar_ismoving) goto END;
    trk = deadbeef->streamer_get_playing_track ();
    if (!trk || deadbeef->pl_get_item_duration (trk) < 0) {
        if (trk) {
            deadbeef->pl_item_unref (trk);
        }
        if (headerbarui_flags.hide_seekbar_on_streaming)
            seekbar_isvisible = FALSE;
        else
            headerbarui_reset_seekbar_cb(NULL);
        goto END;
    }
    if (deadbeef->pl_get_item_duration (trk) > 0) {
        headerbarui_adjust_range(GTK_RANGE (headerbar_seekbar),
            deadbeef->streamer_get_playpos (), //value
            0, // lower
            deadbeef->pl_get_item_duration (trk), // upper
            1, // step_increment
            10, // page_increment
            1); // page_size

        gtk_scale_set_draw_value(GTK_SCALE(headerbar_seekbar), TRUE);
        seekbar_isvisible = TRUE;
    }
    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
END:
    if (!headerbarui_flags.seekbar_minimized) gtk_widget_set_visible(headerbar_seekbar, seekbar_isvisible && headerbarui_flags.show_seek_bar);
    return !headerbar_stoptimer;
}


static
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

static
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
        GtkWidget *menuitem;
        menuitem = gtk_menu_item_new_with_mnemonic(gtk_menu_item_get_label(GTK_MENU_ITEM(l->data)));
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), gtk_menu_item_get_submenu(GTK_MENU_ITEM(l->data)));
        gtk_widget_show(menuitem);
        gtk_container_add(GTK_CONTAINER(menu), GTK_WIDGET(menuitem));
    }
    g_list_free(children);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON (headerbar_menubtn), GTK_WIDGET(menu));
}

static gint
seekbar_width () {
    int button_size = 38; // can maybe be read dynamic, depending on padding of theme
    // Min size calculated by basic static elements (prev, play/pause, next, menu) including 3 possible window decoration buttons
    // For every optional button extra width is added.
    int min_size_fixed_content = button_size * 9;
    if (headerbarui_flags.show_stop_button) {
        min_size_fixed_content += button_size;
    }
    if (headerbarui_flags.show_volume_button) {
        min_size_fixed_content += button_size;
    }
    if (headerbarui_flags.show_preferences_button) {
        min_size_fixed_content += button_size;
    }
    if (!headerbarui_flags.combined_playpause) {
        min_size_fixed_content += button_size;
    }
    int min_size_seekbar = 140;
    int min_size_title = 100;
    int required_width = min_size_fixed_content + min_size_title + min_size_seekbar;

    if (mainwin_width < required_width) {
        return 0;
    } else {
        int remaining_width = mainwin_width - required_width;
        return min_size_seekbar + remaining_width * 0.7;
    }
}

static gboolean
mainwindow_resize (GtkWindow *mainwindow,
                   GdkEventConfigure *event,
                   gpointer pointer) {
    if (headerbarui_flags.show_seek_bar && seekbar_isvisible && event->width != mainwin_width) {
        mainwin_width = event->width;

        int width = seekbar_width();

        if (width == 0) {
            headerbarui_flags.seekbar_minimized = TRUE;
            gtk_widget_hide (headerbar_seekbar);
        } else {
            headerbarui_flags.seekbar_minimized = FALSE;
            gtk_widget_set_size_request (headerbar_seekbar,
                width,
                -1);
            gtk_widget_show (headerbar_seekbar);
        }

    }
    return FALSE;
}

static void
action_design_mode_change_state(GSimpleAction *simple, GVariant *value, gpointer user_data)
{
    gboolean state = g_variant_get_boolean (value);
    GtkCheckMenuItem *designmode_menu_item = GTK_CHECK_MENU_ITEM (lookup_widget (gtkui_plugin->get_mainwin(), "design_mode1"));
    gtk_check_menu_item_set_active (designmode_menu_item, state);

    gtkui_plugin->w_set_design_mode (state);

    g_simple_action_set_state ( simple,  value);
}

static void
design_mode_menu_item_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    GSimpleAction *designmode_action = G_SIMPLE_ACTION (user_data);

    g_simple_action_set_state (designmode_action, g_variant_new_boolean (act));
}

static GActionGroup *
create_action_group(void)
{
    const GActionEntry entries[] = {
        {"designmode", NULL, NULL, "false", action_design_mode_change_state}};
    GSimpleActionGroup *group;

    group = g_simple_action_group_new();
    g_action_map_add_action_entries(G_ACTION_MAP(group), entries, G_N_ELEMENTS(entries), NULL);

    return G_ACTION_GROUP(group);
}

void
dup_gtkui_exec_action_14 (DB_plugin_action_t *action, int cursor) {
    // Plugin can handle all tracks by itself
    if (action->flags & DB_ACTION_CAN_MULTIPLE_TRACKS)
    {
        action->callback (action, NULL);
        return;
    }

    // For single-track actions just invoke it with first selected track
    if (!(action->flags & DB_ACTION_MULTIPLE_TRACKS))
    {
        if (cursor == -1) {
            cursor = deadbeef->pl_get_cursor (PL_MAIN);
        }
        if (cursor == -1) 
        {
            return;
        }
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
        action->callback (action, it);
        deadbeef->pl_item_unref (it);
        return;
    }

    //We end up here if plugin won't traverse tracks and we have to do it ourselves
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            action->callback (action, it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

static void
action_activate(GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
    DB_plugin_action_t *dbaction = (DB_plugin_action_t *) g_object_get_data (G_OBJECT (simple), "deadbeefaction");
    if (dbaction->callback) {
        dup_gtkui_exec_action_14 (dbaction, -1);
    }
    else if (dbaction->callback2) {
        dbaction->callback2 (dbaction, DDB_ACTION_CTX_MAIN);
    }
}

static GActionGroup *
create_action_group_deadbeef(void)
{
    GSimpleActionGroup *group;
    GSimpleAction *action;

    group = g_simple_action_group_new();

    // add new
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;

    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *dbactions = plugins[i]->get_actions (NULL);
        DB_plugin_action_t *dbaction;

        for (dbaction = dbactions; dbaction; dbaction = dbaction->next)
        {
            char *tmp = NULL;

            if (dbaction->callback2 && dbaction->flags & DB_ACTION_COMMON) {
                action = g_simple_action_new (dbaction->name, NULL);
                g_object_set_data (G_OBJECT (action), "deadbeefaction", dbaction);
                g_signal_connect (action, "activate", action_activate, NULL);
                g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
                g_debug ("Action added %s", dbaction->name);
            }

        }
    }

    return G_ACTION_GROUP (group);
}

void window_init_hook (void *userdata) {
    GtkWindow *mainwin;
    GtkWidget *menubar;
    GtkBuilder *builder;

    mainwin = GTK_WINDOW (gtkui_plugin->get_mainwin ());

    menubar = lookup_widget (GTK_WIDGET(mainwin), "menubar");
    g_assert_nonnull(mainwin);
    g_assert_nonnull(menubar);

    builder = gtk_builder_new_from_resource("/org/deadbeef/headerbarui/headerbar.ui");
    gtk_builder_add_from_resource (builder, "/org/deadbeef/headerbarui/menu.ui", NULL);
    headerbar = GTK_BUILDER_GET_WIDGET(builder, "headerbar1");
    volbutton = GTK_BUILDER_GET_WIDGET(builder, "volumebutton1");
    headerbar_menubtn =  GTK_BUILDER_GET_WIDGET(builder, "menubutton1");
    headerbar_seekbar = GTK_BUILDER_GET_WIDGET(builder, "scale1");
    headerbar_playbtn = GTK_BUILDER_GET_WIDGET(builder, "playbtn");
    headerbar_pausebtn = GTK_BUILDER_GET_WIDGET(builder, "pausebtn");
    headerbar_stopbtn = GTK_BUILDER_GET_WIDGET(builder, "stopbtn");
    headerbar_prefsbtn = GTK_BUILDER_GET_WIDGET(builder, "prefsbtn");
    headerbar_designmodebtn = GTK_BUILDER_GET_WIDGET(builder, "designmodebtn");
    GMenuModel *menumodel = G_MENU_MODEL (gtk_builder_get_object (builder, "file-menu"));

    GtkWidget *file_menu_btn = GTK_MENU_BUTTON (gtk_builder_get_object (builder, "file_menu_btn"));
    //gtk_menu_button_set_use_popover (GTK_MENU_BUTTON (file_menu_btn), FALSE);
    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (file_menu_btn), menumodel);

    GActionGroup *group = create_action_group();
    gtk_widget_insert_action_group (headerbar, "win", group);

    GAction *designmode_action = g_action_map_lookup_action (G_ACTION_MAP (group), "designmode");

    g_signal_connect_after (G_OBJECT (lookup_widget (gtkui_plugin->get_mainwin(), "design_mode1")),
        "activate", G_CALLBACK (design_mode_menu_item_activate), designmode_action);

    GActionGroup *deadbeef_action_group = create_action_group_deadbeef();    
    gtk_widget_insert_action_group (headerbar, "db", deadbeef_action_group);

    g_object_set(G_OBJECT(headerbar), "spacing", headerbarui_flags.button_spacing, NULL);
    gtk_widget_show(headerbar);

    gtk_window_set_titlebar(mainwin, GTK_WIDGET(headerbar));

    if (!headerbarui_flags.embed_menubar)
    {
        gtk_widget_hide(menubar);

        headerbarui_update_menubutton();

        gtk_widget_set_can_focus(headerbar_menubtn, FALSE);
        gtk_widget_show (headerbar_menubtn);
    } else {
        gtk_widget_destroy(headerbar_menubtn);
        gtk_widget_reparent(menubar, headerbar);
        gtk_container_child_set(GTK_CONTAINER(headerbar), menubar, "position", 0, NULL);
    }


    if (!headerbarui_flags.combined_playpause) {
        gtk_widget_show(headerbar_playbtn);
        gtk_widget_show(headerbar_pausebtn);
    }

    gtk_widget_set_visible(headerbar_prefsbtn, headerbarui_flags.show_preferences_button);

    float volume = deadbeef->volume_get_min_db()-deadbeef->volume_get_db();
    g_assert_false((volume>0));
    gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON (volbutton),
        gtk_adjustment_new (volume, 0, -deadbeef->volume_get_min_db (), 5, 5, 0));

    gtk_widget_show(volbutton);

    gtk_builder_add_callback_symbols(builder,
        "on_volbutton_value_changed", (GCallback)on_volbutton_value_changed,
        "on_seekbar_format_value", on_seekbar_format_value,
        "on_seekbar_button_press_event", on_seekbar_button_press_event,
        "on_seekbar_button_release_event", on_seekbar_button_release_event,
        "on_seekbar_value_changed", on_seekbar_value_changed,
        NULL);
    gtk_builder_connect_signals(builder, NULL);

    gtk_window_get_size (mainwin, &mainwin_width, NULL);
    gtk_widget_set_size_request (headerbar_seekbar, seekbar_width (), -1);
    g_signal_connect (G_OBJECT(mainwin),
        "configure-event",
        G_CALLBACK(mainwindow_resize),
        NULL);

}


static
void headerbarui_getconfig()
{
    headerbarui_flags.disable = deadbeef->conf_get_int ("headerbarui.disable", 0);
    headerbarui_flags.embed_menubar = deadbeef->conf_get_int ("headerbarui.embed_menubar", 0);
    headerbarui_flags.show_seek_bar = deadbeef->conf_get_int ("headerbarui.show_seek_bar", 1);
    if (headerbarui_flags.show_seek_bar)
        headerbarui_flags.hide_seekbar_on_streaming = deadbeef->conf_get_int ("headerbarui.hide_seekbar_on_streaming", 0);
    else
        headerbarui_flags.hide_seekbar_on_streaming = FALSE;
    headerbarui_flags.combined_playpause = deadbeef->conf_get_int ("headerbarui.combined_playpause", 1);
    headerbarui_flags.show_stop_button = deadbeef->conf_get_int ("headerbarui.show_stop_button", 1);
    headerbarui_flags.show_volume_button = deadbeef->conf_get_int ("headerbarui.show_volume_button", 1);
    headerbarui_flags.show_preferences_button = deadbeef->conf_get_int ("headerbarui.show_preferences_button", 0);
    headerbarui_flags.show_designmode_button = deadbeef->conf_get_int ("headerbarui.show_designmode_button", 0);
    headerbarui_flags.button_spacing = deadbeef->conf_get_int ("headerbarui.button_spacing", 6);
}

static
int headerbarui_connect() {
    headerbarui_getconfig();
    if (headerbarui_flags.disable) return 1;
    gtkui_plugin = (ddb_gtkui_t *) deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    if (gtkui_plugin) {
        if (gtkui_plugin->gui.plugin.version_major >= 2) {
            gtkui_plugin->add_window_init_hook (window_init_hook, NULL);
            return 0;
        }
    }
    return -1;
}

static
gboolean
headerbarui_volume_changed(gpointer user_data)
{
    float volume = deadbeef->volume_get_min_db()-deadbeef->volume_get_db();
    if (volume > 0) volume = 0;

    GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA);
    GQuark detail = g_quark_from_static_string("value_changed");
    g_signal_handlers_block_matched ((gpointer)volbutton, mask, detail, 0, NULL, NULL, NULL);
    gtk_scale_button_set_value( GTK_SCALE_BUTTON (volbutton), -volume );
    g_signal_handlers_unblock_matched ((gpointer)volbutton, mask, detail, 0, NULL, NULL, NULL);

    return FALSE;
}


void
playpause_update(int state) {
    if (headerbarui_flags.combined_playpause) {
        switch (state) {
            case OUTPUT_STATE_PLAYING:
            gtk_widget_show(headerbar_pausebtn);
            gtk_widget_hide(headerbar_playbtn);
            break;
            case OUTPUT_STATE_STOPPED:
            case OUTPUT_STATE_PAUSED:
            gtk_widget_show(headerbar_playbtn);
            gtk_widget_hide(headerbar_pausebtn);
            break;
        }
    } else {
        gtk_widget_show(headerbar_playbtn);
        gtk_widget_show(headerbar_pausebtn);
    }
}

static
gboolean
headerbarui_configchanged_cb(gpointer user_data)
{
    gtk_widget_set_visible(headerbar_seekbar, headerbarui_flags.show_seek_bar && seekbar_isvisible);
    gtk_widget_set_visible(headerbar_stopbtn, headerbarui_flags.show_stop_button);
    gtk_widget_set_visible(volbutton, headerbarui_flags.show_volume_button);
    gtk_widget_set_visible(headerbar_prefsbtn, headerbarui_flags.show_preferences_button);
    gtk_widget_set_visible(headerbar_prefsbtn, headerbarui_flags.show_preferences_button);
    gtk_widget_set_visible(headerbar_designmodebtn, headerbarui_flags.show_designmode_button);
    g_object_set(G_OBJECT(headerbar), "spacing", headerbarui_flags.button_spacing, NULL);
    playpause_update(OUTPUT_STATE_STOPPED);

    return FALSE;
}

static int
headerbarui_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    if (id != DB_EV_CONFIGCHANGED && headerbarui_flags.disable) return 0;
    switch (id) {
    case DB_EV_SONGSTARTED:
        headerbar_stoptimer = 0;
        headerbar_timer = g_timeout_add (1000/gtkui_get_gui_refresh_rate (), headerbarui_update_seekbar_cb, NULL);
        break;
    case DB_EV_SONGFINISHED:
        headerbar_stoptimer = 1;
        break;
    case DB_EV_CONFIGCHANGED:
        headerbarui_getconfig();
        g_idle_add (headerbarui_configchanged_cb, NULL);
        g_idle_add (headerbarui_volume_changed, NULL);
        break;
    case DB_EV_VOLUMECHANGED:
        g_idle_add (headerbarui_volume_changed, NULL);
        break;
    }
    return 0;
}

static const char settings_dlg[] =
    "property \"Disable plugin (requires restart)\" checkbox headerbarui.disable 0;\n"
    "property \"Embed menubar instead of showing hamburger button (requires restart)\" checkbox headerbarui.embed_menubar 0;\n"
    "property \"Button spacing (pixels)\" entry headerbarui.button_spacing 6;\n"
    "property \"Use combined play/pause button\" checkbox headerbarui.combined_playpause 1;\n"
    "property \"Show seekbar\" checkbox headerbarui.show_seek_bar 1;\n"
    "property \"Hide seekbar on streaming\" checkbox headerbarui.hide_seekbar_on_streaming 0;\n"
    "property \"Show stop button\" checkbox headerbarui.show_stop_button 1;\n"
    "property \"Show volume button\" checkbox headerbarui.show_volume_button 1;\n"
    "property \"Show preferences button\" checkbox headerbarui.show_preferences_button 0;\n"
    "property \"Show design mode button\" checkbox headerbarui.show_designmode_button 0;\n"
;

static DB_misc_t plugin = {
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 9, // using 10 apis if available
#if (DDB_API_LEVEL >= 10)
    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
#endif
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
