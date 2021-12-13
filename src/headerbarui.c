/*
    Headerbar UI plugin for the DeaDBeeF audio player

    Copyright (C) 2015-2021 Nicolai Syvertsen <saivert@gmail.com>
    
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include "gio/gmenu.h"
#include "gio/gmenumodel.h"
#include "resources.h"
#include "headerbarui.h"
#include <math.h>

DB_functions_t *deadbeef;
static DB_misc_t plugin;

static ddb_gtkui_t *gtkui_plugin;

GtkWidget *mainwin;
GtkWidget *headerbar;
GtkWidget *volbutton;
GtkWidget *headerbar_seekbar;
GtkWidget *headerbar_playbtn;
GtkWidget *headerbar_pausebtn;
GtkWidget *headerbar_stopbtn;
GtkWidget *headerbar_menubtn;
GtkWidget *headerbar_prefsbtn;
GtkWidget *headerbar_designmodebtn;
GtkWidget *headerbar_titlelabel;
GtkWidget *headerbar_seekbarbox;
GtkWidget *headerbar_playbacktimelabel;
GtkWidget *headerbar_durationlabel;
GtkWidget *headerbar_prevbtn;
GtkWidget *headerbar_nextbtn;
GtkWidget *headerbar_playback_button_box;
GtkWidget *headerbar_app_menu_btn;
GtkWidget *headerbar_add_menu_btn;
GtkWidget *headerbar_playback_menu_btn;

#define GTK_BUILDER_GET_WIDGET(builder, name) (GtkWidget *)gtk_builder_get_object(builder, name)

guint headerbar_timer;

gboolean seekbar_ismoving = FALSE;
gboolean seekbar_isvisible = FALSE;
gboolean headerbar_stoptimer = FALSE;

static struct headerbarui_flag_s {
    gboolean disable:1;
    gboolean embed_menubar:1;
    gboolean show_seek_bar:1;
    gboolean seekbar_minimized:1;
    gboolean hide_seekbar_on_streaming:1;
    gboolean combined_playpause:1;
    gboolean show_stop_button:1;
    gboolean show_volume_button:1;
    gboolean show_preferences_button:1;
    gboolean show_designmode_button:1;
    gboolean show_time_remaining:1;
    gboolean hide_playback_buttons:1;
    gboolean new_app_menu:1;
    gboolean show_add_button:1;
    gboolean show_playback_button:1;
    gboolean show_playback_button_prev:1;
    int button_spacing;
} headerbarui_flags;

static char *
format_time (float t, char *dur, int size);

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
    deadbeef->volume_set_amp (value*value*value);
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
    char buf[100];
    gdouble value = gtk_range_get_value(GTK_RANGE(headerbar_seekbar));
    if (seekbar_ismoving) {
        gtk_label_set_text (GTK_LABEL (headerbar_playbacktimelabel), format_time(value, buf, sizeof(buf)));
        return;
    }
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

static
gboolean
on_durationlabel_button_release_event (GtkScale *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
    headerbarui_flags.show_time_remaining = !headerbarui_flags.show_time_remaining;
    deadbeef->conf_set_int ("headerbarui.show_time_remaining", headerbarui_flags.show_time_remaining);
    return FALSE;
}

static char *
format_time (float t, char *dur, int size) {
    if (t >= 0) {
        //t = roundf (t);
        int hourdur = t / (60 * 60);
        int mindur = (t - hourdur * 60 * 60) / 60;
        int secdur = t - hourdur*60*60 - mindur * 60;

        if (hourdur) {
            snprintf (dur, size, "%d:%02d:%02d", hourdur, mindur, secdur);
        }
        else {
            snprintf (dur, size, "%d:%02d", mindur, secdur);
        }
    }
    else {
        strcpy (dur, "∞");
    }
    return dur;
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

    return FALSE;
}

void
playpause_update(int state);

static void
set_seekbar_text(float time, float duration)
{
    char buf[100];
    gtk_label_set_text (GTK_LABEL (headerbar_playbacktimelabel), format_time(time, buf, sizeof(buf)));
    if (headerbarui_flags.show_time_remaining) {
        duration -= time;
        buf[0] = '-';
        format_time(roundf(duration), buf+1, sizeof(buf)-1);
    } else {
        format_time(roundf(duration), buf, sizeof(buf));
    }
    gtk_label_set_text (GTK_LABEL (headerbar_durationlabel), buf);
}

static
gboolean
headerbarui_update_seekbar_cb(gpointer user_data)
{
    DB_playItem_t *trk;
    DB_output_t *out;
    seekbar_isvisible = TRUE;

    trk = deadbeef->streamer_get_playing_track ();
    if (!trk) {
        playpause_update(FALSE);
        seekbar_isvisible = FALSE;
        goto END;
    }

    out = deadbeef->get_output();
    if (out) {
        playpause_update(out->state());
        if (out->state() == OUTPUT_STATE_STOPPED) {
            seekbar_isvisible = FALSE;
            goto END;
        }
    }

    if (seekbar_ismoving) goto END;
    set_seekbar_text(deadbeef->streamer_get_playpos (), deadbeef->pl_get_item_duration (trk));
    if (deadbeef->pl_get_item_duration (trk) < 0) {
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

        seekbar_isvisible = TRUE;
    }

END:
    if (trk) {
        deadbeef->pl_item_unref (trk);
    }

    if (!headerbarui_flags.seekbar_minimized) gtk_widget_set_visible(headerbar_seekbarbox, seekbar_isvisible && headerbarui_flags.show_seek_bar);
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
    GtkWidget *menubar;
    static GtkMenu *menu;

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

static
void
on_title_child_size_allocate (GtkWidget* self, GtkAllocation* allocation, gpointer user_data)
{
    if (headerbarui_flags.show_seek_bar && seekbar_isvisible ) {
        if (allocation->width < 400) {
            headerbarui_flags.seekbar_minimized = TRUE;
            gtk_widget_hide (headerbar_seekbarbox);
        } else {
            headerbarui_flags.seekbar_minimized = FALSE;
            gtk_widget_show (headerbar_seekbarbox);
        }
    }
}

static void
action_design_mode_change_state(GSimpleAction *simple, GVariant *value, gpointer user_data)
{
    gboolean state = g_variant_get_boolean (value);
    GtkCheckMenuItem *designmode_menu_item = GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "design_mode1"));
    gtk_check_menu_item_set_active (designmode_menu_item, state);

    gtkui_plugin->w_set_design_mode (state);

    g_simple_action_set_state ( simple,  value);
}

static void
action_about_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
   GtkWidget *about_item = lookup_widget(mainwin, "about1");
   gtk_menu_item_activate(GTK_MENU_ITEM(about_item));
}

// Toggle log action
static void
action_toggle_log_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    GtkWidget *view_log_item = lookup_widget (mainwin, "view_log");
    gtk_menu_item_activate (GTK_MENU_ITEM(view_log_item));
    gboolean checked = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(view_log_item));
    g_simple_action_set_state (action,  g_variant_new_boolean(checked));
}

// Toggle EQ action
static void
action_toggle_eq_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    GtkWidget *menuitem = lookup_widget (mainwin, "view_eq");
    gtk_menu_item_activate (GTK_MENU_ITEM(menuitem));
    gboolean checked = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(menuitem));
    g_simple_action_set_state (action,  g_variant_new_boolean(checked));
}

// Toggle statusbar action
static void
action_toggle_statusbar_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    GtkWidget *menuitem = lookup_widget (mainwin, "view_status_bar");
    gtk_menu_item_activate (GTK_MENU_ITEM(menuitem));
    gboolean checked = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(menuitem));
    g_simple_action_set_state (action,  g_variant_new_boolean(checked));
}

// Toggle menu action
static void
action_toggle_menu_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    GtkWidget *menubar = lookup_widget (mainwin, "menubar");
    int val = 1-deadbeef->conf_get_int ("gtkui.show_menu", 1);
    val ? gtk_widget_show (menubar) : gtk_widget_hide (menubar);
    deadbeef->conf_set_int ("gtkui.show_menu", val);

    g_simple_action_set_state (action,  g_variant_new_boolean(val));
}

// Copy item action
static void
action_copy_item_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        gtkui_plugin->copy_selection (plt, DDB_ACTION_CTX_SELECTION);
        deadbeef->plt_unref (plt);
    }
}

static void
action_cut_item_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        gtkui_plugin->cut_selection (plt, DDB_ACTION_CTX_SELECTION);
        deadbeef->plt_unref (plt);
    }
}

static void
action_paste_item_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        gtkui_plugin->paste_selection (plt, DDB_ACTION_CTX_SELECTION);
        deadbeef->plt_unref (plt);
    }
}

static gboolean
toggle_and_get_active_menu_item(const gchar *glade_id)
{
    GtkWidget *menuitem = lookup_widget (mainwin, glade_id);
    gtk_menu_item_activate(GTK_MENU_ITEM(menuitem));
    // gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
    return gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(menuitem));
}

// Shuffle mode

static void
action_shuffle_mode_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    const gchar *mode = g_variant_get_string (parameter, NULL);
    int update_state = 0;
    if (!g_strcmp0(mode, "off")) {
        update_state = toggle_and_get_active_menu_item ("order_linear");
    } else if (!g_strcmp0(mode, "tracks")) {
        update_state = toggle_and_get_active_menu_item ("order_shuffle");
    } else if (!g_strcmp0(mode, "albums")) {
        update_state = toggle_and_get_active_menu_item ("order_shuffle_albums");
    } else if (!g_strcmp0(mode, "random")) {
        update_state = toggle_and_get_active_menu_item ("order_random");
    }

    if (update_state) {
        g_simple_action_set_state (action, parameter);
    }
}

static void
order_linear_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GSimpleAction *action = G_SIMPLE_ACTION (user_data);
    g_simple_action_set_state (action, g_variant_new_string ("off"));
}

static void
order_shuffle_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GSimpleAction *action = G_SIMPLE_ACTION (user_data);
    g_simple_action_set_state (action, g_variant_new_string ("tracks"));
}

static void
order_shuffle_albums_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GSimpleAction *action = G_SIMPLE_ACTION (user_data);
    g_simple_action_set_state (action, g_variant_new_string ("albums"));
}

static void
order_random_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GSimpleAction *action = G_SIMPLE_ACTION (user_data);
    g_simple_action_set_state (action, g_variant_new_string ("random"));
}

// Repeat mode

static void
action_repeat_mode_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    const gchar *mode = g_variant_get_string (parameter, NULL);
    int update_state = 0;
    if (!g_strcmp0(mode, "off")) {
        update_state = toggle_and_get_active_menu_item ("loop_disable");
    } else if (!g_strcmp0(mode, "single")) {
        update_state = toggle_and_get_active_menu_item ("loop_single");
    } else if (!g_strcmp0(mode, "all")) {
        update_state = toggle_and_get_active_menu_item ("loop_all");
    }

    if (update_state) {
        g_simple_action_set_state (action, parameter);
    }
}

static void
loop_disable_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GSimpleAction *action = G_SIMPLE_ACTION (user_data);
    g_simple_action_set_state (action, g_variant_new_string ("off"));
}

static void
loop_single_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GSimpleAction *action = G_SIMPLE_ACTION (user_data);
    g_simple_action_set_state (action, g_variant_new_string ("single"));
}

static void
loop_all_albums_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GSimpleAction *action = G_SIMPLE_ACTION (user_data);
    g_simple_action_set_state (action, g_variant_new_string ("all"));
}

static void
action_scroll_follows_playback_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    gboolean active = toggle_and_get_active_menu_item ("scroll_follows_playback");
    g_simple_action_set_state (action, g_variant_new_boolean (active)); 
}

static void
action_cursor_follows_playback_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    gboolean active = toggle_and_get_active_menu_item ("cursor_follows_playback");
    g_simple_action_set_state (action, g_variant_new_boolean (active)); 
}

static void
action_stop_after_current_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    gboolean active = toggle_and_get_active_menu_item ("stop_after_current");
    g_simple_action_set_state (action, g_variant_new_boolean (active)); 
}

static void
action_stop_after_album_activate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    gboolean active = toggle_and_get_active_menu_item ("stop_after_album");
    g_simple_action_set_state (action, g_variant_new_boolean (active)); 
}

static void
common_checked_menuitem_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    GSimpleAction *action = G_SIMPLE_ACTION (user_data);

    g_simple_action_set_state (action, g_variant_new_boolean (act));
}

static gboolean
get_checked_menu_item_active(gchar *glade_id)
{
    GtkWidget *menuitem = lookup_widget(mainwin, glade_id);
    if (!menuitem || !gtk_widget_get_visible(menuitem)) {
        return FALSE;
    }
    return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
}

// Actions that have no matching deadbeef action or need to sync with deadbeef's own menus
static GActionGroup *
create_action_group(void)
{
    const GActionEntry entries[] = {
        {"designmode", NULL, NULL, "false", action_design_mode_change_state},
        {"toggle_log", action_toggle_log_activate, NULL, "false", NULL},
        {"toggle_eq", action_toggle_eq_activate, NULL,
            get_checked_menu_item_active("view_eq") ? "true" : "false", NULL},
        {"toggle_statusbar", action_toggle_statusbar_activate, NULL,
            get_checked_menu_item_active("view_status_bar") ? "true" : "false", NULL},
        {"toggle_menu", action_toggle_menu_activate, NULL, "false", NULL},
        {"copy_item", action_copy_item_activate, NULL, NULL, NULL},
        {"cut_item", action_cut_item_activate, NULL, NULL, NULL},
        {"paste_item", action_paste_item_activate, NULL, NULL, NULL},
        {"about", action_about_activate, NULL, NULL, NULL},
        {"shufflemode", action_shuffle_mode_activate, "s", "'off'", NULL},
        {"repeatmode", action_repeat_mode_activate, "s", "'off'", NULL},
        {"scroll_follows_playback", action_scroll_follows_playback_activate, NULL,
            get_checked_menu_item_active("scroll_follows_playback") ? "true" : "false", NULL},
        {"cursor_follows_playback", action_cursor_follows_playback_activate, NULL,
            get_checked_menu_item_active("cursor_follows_playback") ? "true" : "false", NULL},
        {"stop_after_current", action_stop_after_current_activate, NULL,
            get_checked_menu_item_active("stop_after_current") ? "true" : "false", NULL},
        {"stop_after_album", action_stop_after_album_activate, NULL,
            get_checked_menu_item_active("stop_after_album") ? "true" : "false", NULL},
    };
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
        if (parameter && g_variant_is_of_type(parameter, G_VARIANT_TYPE_INT32)) {
            int value = g_variant_get_int32(parameter);
            dbaction->callback2 (dbaction, value);
        } else {
            dbaction->callback2 (dbaction, DDB_ACTION_CTX_MAIN);
        }
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
            if (dbaction->callback2 && dbaction->flags & (DB_ACTION_COMMON|DB_ACTION_MULTIPLE_TRACKS)) {
                action = g_simple_action_new (dbaction->name, (dbaction->flags & DB_ACTION_MULTIPLE_TRACKS) ? G_VARIANT_TYPE_INT32 : NULL);
                g_object_set_data (G_OBJECT (action), "deadbeefaction", dbaction);
                g_signal_connect (action, "activate", G_CALLBACK (action_activate), NULL);
                g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
                g_debug ("Action added %s", dbaction->name);
            }

        }
    }

    return G_ACTION_GROUP (group);
}

void mainwindow_settitle(GtkWidget* widget,
                    GParamSpec* property,
                    gpointer data)
{
    // Since we use a custom title widget we need to reimplement copying the window title
    gtk_label_set_text (GTK_LABEL (headerbar_titlelabel), gtk_window_get_title (GTK_WINDOW (mainwin)));
}


static void
hookup_action_to_menu_item(GActionMap *map, const gchar *action_name, const gchar *glade_id)
{
    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (map), action_name);

    g_signal_connect_after (G_OBJECT (lookup_widget (mainwin, glade_id)),
        "activate", G_CALLBACK (common_checked_menuitem_activate), action);
}

static void
hookup_action_to_radio_menu_item(GActionMap *map, const gchar *action_name, GCallback event_handler, const gchar *glade_id)
{
    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (map), action_name);

    g_signal_connect_after (G_OBJECT (lookup_widget (mainwin, glade_id)), "activate", event_handler, action);
}

#ifdef DEBUG
gboolean treemodelprintforeach(GtkTreeModel* model,
                                    GtkTreePath* path,
                                    GtkTreeIter* iter,
                                    gpointer data)
{
    GValue val = {0,};
    gtk_tree_model_get_value (GTK_TREE_MODEL (model), iter, 0, &val);
    const char *n = g_value_get_string (&val);
    gchar *pathstring = gtk_tree_path_to_string(path);
    trace ("Current iter %s %s\n", n, pathstring);
    g_free(pathstring);
    return FALSE;
}
#endif

static void
construct_menu(GtkTreeStore *store, GMenuModel *menumodel, GtkTreeIter *root)
{
    GtkTreeIter i;
    gboolean res = gtk_tree_model_iter_children(GTK_TREE_MODEL (store), &i, root);
    if (!res) return;
    do {
        GValue titleval = {0,};
        GValue actionval = {0,};
        gtk_tree_model_get_value (GTK_TREE_MODEL (store), &i, 0, &titleval);
        gtk_tree_model_get_value (GTK_TREE_MODEL (store), &i, 1, &actionval);
        const gchar *t = g_value_get_string (&titleval);
        const gchar *a = g_value_get_string (&actionval);
        if (a != NULL) {
            trace ("Found menu item %s, %s\n", t, a);
            char act[100];
            snprintf(act, sizeof(act), "plg.%s", a);
            g_menu_append(G_MENU(menumodel), t, act);
        }
        if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL (store), &i)) {
            GMenu *submenu = g_menu_new();
            construct_menu(store, G_MENU_MODEL(submenu), &i);
            g_menu_append_submenu(G_MENU(menumodel), t, G_MENU_MODEL(submenu));
            g_object_unref (submenu);
        }
    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &i));
}

static void
add_action_menuitems(GtkTreeStore *store, GMenuModel *menumodel, const char *menu_to_add_to)
{
    GtkTreeIter i;

    gboolean res = gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &i, NULL);
    if (res) {
        do {
            GValue val = {0,};
            gtk_tree_model_get_value (GTK_TREE_MODEL (store), &i, 0, &val);
            const gchar *n = g_value_get_string (&val);
            if (!strcmp(n, menu_to_add_to)) {
                trace ("Found menu %s\n", n);
                construct_menu(store, menumodel, &i);
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &i));
    }
}

static const char *
get_display_action_title (const char *title) {
    const char *t = title + strlen (title) - 1;
    while (t > title) {
        if (*t != '/' || *(t-1) == '\\') {
            t--;
            continue;
        }
        t++;
        break;
    }
    return t;
}

#ifndef strdupa
# define strdupa(s)							      \
    ({									      \
      const char *old = (s);					      \
      size_t len = strlen (old) + 1;				      \
      char *newstr = (char *) alloca (len);			      \
      (char *) memcpy (newstr, old, len);				      \
    })
#endif

static const char *
action_tree_append (const char *title, GtkTreeStore *store, GtkTreeIter *root_iter, GtkTreeIter *iter) {
    char *t = strdupa (title);
    char *p = t;
    GtkTreeIter i;
    GtkTreeIter newroot;
    for (;;) {
        char *s = strchr (p, '/');
        // find unescaped forward slash
        if (s == p) {
            p++;
            continue;
        }
        if (s && s > p && *(s-1) == '\\') {
            p = s + 1;
            continue;
        }
        if (!s) {
            break;
        }
        *s = 0;
        // find iter in the current root with name==p
        gboolean res = gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &i, root_iter);
        if (!res) {
            gtk_tree_store_append (store, &i, root_iter);
            gtk_tree_store_set (store, &i, 0, p, 1, NULL, 2, -1, -1);
            memcpy (&newroot, &i, sizeof (GtkTreeIter));
            root_iter = &newroot;
        }
        else {
            int found = 0;
            do {
                GValue val = {0,};
                gtk_tree_model_get_value (GTK_TREE_MODEL (store), &i, 0, &val);
                const char *n = g_value_get_string (&val);
                if (n && !strcmp (n, p)) {
                    memcpy (&newroot, &i, sizeof (GtkTreeIter));
                    root_iter = &newroot;
                    found = 1;
                    break;
                }
            } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &i));
            if (!found) {
                gtk_tree_store_append (store, &i, root_iter);
                gtk_tree_store_set (store, &i, 0, p, 1, NULL, 2, -1, -1);
                memcpy (&newroot, &i, sizeof (GtkTreeIter));
                root_iter = &newroot;
            }
        }

        p = s+1;
    }
    gtk_tree_store_append (store, iter, root_iter);
    return get_display_action_title (title);
}


static void
unescape_forward_slash (const char *src, char *dst, int size) {
    char *start = dst;
    while (*src) {
        if (dst - start >= size - 1) {
            break;
        }
        if (*src == '\\' && *(src+1) == '/') {
            src++;
        }
        *dst++ = *src++;
    }
    *dst = 0;
}

static int
menu_add_action_items(GSimpleActionGroup *actiongroup, GtkTreeStore **actions_store_out) {
    ddb_action_context_t action_context = DDB_ACTION_CTX_MAIN;
    int hide_remove_from_disk = deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0);
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;
    int added_entries = 0;

    // traverse all plugins and collect all exported actions to treeview
    // column0: title
    // column1: ID (invisible)
    // column2: ctx (invisible
    GtkTreeStore *actions_store = gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    GtkTreeIter *rootiter=NULL;


    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (NULL);
        DB_plugin_action_t *action;

        for (action = actions; action; action = action->next)
        {
            // if (!action->name || !action->title) {
            //     continue;
            // }

            if (action->name && !strcmp (action->name, "delete_from_disk") && hide_remove_from_disk) {
                continue;
            }

            if (action->flags&DB_ACTION_DISABLED) {
                continue;
            }

            if (!((action->callback2 && (action->flags & DB_ACTION_ADD_MENU)) || action->callback)) {
                continue;
            }

            if (action_context == DDB_ACTION_CTX_SELECTION) {
                if ((action->flags & DB_ACTION_COMMON)
                    || !(action->flags & (DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK))) {
                    continue;
                }
            }

            if (action_context == DDB_ACTION_CTX_PLAYLIST) {
                if (action->flags & DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST) {
                    continue;
                }
                if (action->flags & DB_ACTION_COMMON) {
                    continue;
                }
            }
            else if (action_context == DDB_ACTION_CTX_MAIN) {
                if (!((action->flags & (DB_ACTION_COMMON|DB_ACTION_ADD_MENU)) == (DB_ACTION_COMMON|DB_ACTION_ADD_MENU))) {
                    continue;
                }
                const char *slash_test = action->title;
                while (NULL != (slash_test = strchr (slash_test, '/'))) {
                    if (slash_test && slash_test > action->title && *(slash_test-1) == '\\') {
                        slash_test++;
                        continue;
                    }
                    break;
                }

                if (slash_test == NULL) {
                    continue;
                }
            }

            trace ("Got action #%d %s [%s]\n", added_entries, action->title, action->name);

            char title[100];

            GtkTreeIter iter;
            const char *t;
            t = action_tree_append (action->title, actions_store, rootiter, &iter);
            unescape_forward_slash (t, title, sizeof (title));
            gtk_tree_store_set (actions_store, &iter, 0, title, 1, action->name, 2, DDB_ACTION_CTX_MAIN, -1);

            added_entries++;

            if (!g_action_map_lookup_action(G_ACTION_MAP (actiongroup), action->name)) {
                GSimpleAction *simpleaction;

                simpleaction = g_simple_action_new (action->name, (action->flags & DB_ACTION_MULTIPLE_TRACKS) ? G_VARIANT_TYPE_INT32 : NULL);
                g_object_set_data (G_OBJECT (simpleaction), "deadbeefaction", action);
                g_signal_connect (simpleaction, "activate", G_CALLBACK (action_activate), NULL);
                g_action_map_add_action (G_ACTION_MAP (actiongroup), G_ACTION (simpleaction));
            }

        }
    }
    *actions_store_out = actions_store;
    return added_entries;
}

GMenuModel *file_menu;
GMenuModel *playback_menu;
GMenuModel *app_menu;

static void
update_plugin_actions() {
    GActionGroup *actiongroup;// = gtk_widget_get_action_group(headerbar, "plg");

    actiongroup = G_ACTION_GROUP(g_simple_action_group_new());
    gtk_widget_insert_action_group(headerbar, "plg", actiongroup);

    GtkTreeStore *treestore=NULL;

    GtkBuilder *builder = gtk_builder_new_from_resource("/org/deadbeef/headerbarui/menu.ui");
    file_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "file-menu"));

    playback_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "playback-menu"));
    app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu"));


    if (menu_add_action_items(G_SIMPLE_ACTION_GROUP(actiongroup), &treestore) > 0) {
        GMenuModel *tmpsection;

#ifdef DEBUG
        // gtk_tree_model_foreach (GTK_TREE_MODEL (treestore), treemodelprintforeach, NULL);
#endif

        tmpsection = G_MENU_MODEL(g_menu_new());
        add_action_menuitems(treestore, tmpsection, "File");
        g_menu_insert_section(G_MENU(file_menu), 2, NULL, tmpsection);
        g_object_unref(tmpsection);

        tmpsection = G_MENU_MODEL(g_menu_new());
        add_action_menuitems(treestore, tmpsection, "Playback");
        g_menu_append_section(G_MENU(playback_menu), NULL, tmpsection);
        g_object_unref(tmpsection);

        tmpsection = G_MENU_MODEL(g_menu_new());
        add_action_menuitems(treestore, tmpsection, "Edit");
        g_menu_insert_section(G_MENU(app_menu), 5, NULL, tmpsection);
        g_object_unref(tmpsection);

        g_object_unref (treestore);

    }

    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON(headerbar_add_menu_btn), file_menu);
    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON(headerbar_playback_menu_btn), playback_menu);
    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON(headerbar_app_menu_btn), app_menu);

    if (!headerbarui_flags.show_playback_button) {
        g_menu_insert_submenu(G_MENU(app_menu), 3, "Playback options", playback_menu);
    }

    g_object_unref(builder);

}

void window_init_hook (void *userdata) {
    GtkWidget *menubar;
    GtkBuilder *builder;

    mainwin = gtkui_plugin->get_mainwin ();

    menubar = lookup_widget (GTK_WIDGET(mainwin), "menubar");
    g_assert_nonnull(mainwin);
    g_assert_nonnull(menubar);

    builder = gtk_builder_new_from_resource("/org/deadbeef/headerbarui/headerbar.ui");
    gtk_builder_add_from_resource (builder, "/org/deadbeef/headerbarui/menu.ui", NULL);
    headerbar = GTK_BUILDER_GET_WIDGET(builder, "headerbar1");
    volbutton = GTK_BUILDER_GET_WIDGET(builder, "volumebutton1");
    GtkStyleContext *volctx = gtk_widget_get_style_context(volbutton);
    gtk_style_context_remove_class(volctx, "flat");
    headerbar_menubtn =  GTK_BUILDER_GET_WIDGET(builder, "menubutton1");
    headerbar_app_menu_btn =  GTK_BUILDER_GET_WIDGET(builder, "app_menu_btn");
    headerbar_seekbar = GTK_BUILDER_GET_WIDGET(builder, "seekbar");
    headerbar_playbtn = GTK_BUILDER_GET_WIDGET(builder, "playbtn");
    headerbar_pausebtn = GTK_BUILDER_GET_WIDGET(builder, "pausebtn");
    headerbar_stopbtn = GTK_BUILDER_GET_WIDGET(builder, "stopbtn");
    headerbar_prefsbtn = GTK_BUILDER_GET_WIDGET(builder, "prefsbtn");
    headerbar_designmodebtn = GTK_BUILDER_GET_WIDGET(builder, "designmodebtn");
    headerbar_seekbarbox = GTK_BUILDER_GET_WIDGET(builder, "seekbarbox");
    headerbar_playbacktimelabel = GTK_BUILDER_GET_WIDGET(builder, "playbacktimelabel");
    headerbar_durationlabel = GTK_BUILDER_GET_WIDGET(builder, "durationlabel");
    headerbar_titlelabel = GTK_BUILDER_GET_WIDGET(builder, "titlelabel");
    headerbar_prevbtn = GTK_BUILDER_GET_WIDGET(builder, "prevbtn");
    headerbar_nextbtn = GTK_BUILDER_GET_WIDGET(builder, "nextbtn");
    headerbar_playback_button_box = GTK_BUILDER_GET_WIDGET(builder, "playback_button_box");
    headerbar_add_menu_btn = GTK_BUILDER_GET_WIDGET(builder, "file_menu_btn");
    headerbar_playback_menu_btn = GTK_BUILDER_GET_WIDGET(builder, "playback_menu_btn");

    GActionGroup *group = create_action_group();
    gtk_widget_insert_action_group (headerbar, "win", group);

    GActionGroup *deadbeef_action_group = create_action_group_deadbeef();    
    gtk_widget_insert_action_group (headerbar, "db", deadbeef_action_group);

    update_plugin_actions();


    hookup_action_to_menu_item(G_ACTION_MAP(group), "designmode", "design_mode1");
    hookup_action_to_menu_item(G_ACTION_MAP(group), "toggle_log", "view_log");
    hookup_action_to_menu_item(G_ACTION_MAP(group), "toggle_eq", "view_eq");
    hookup_action_to_menu_item(G_ACTION_MAP(group), "toggle_statusbar", "view_status_bar");
    hookup_action_to_menu_item(G_ACTION_MAP(group), "scroll_follows_playback", "scroll_follows_playback");
    hookup_action_to_menu_item(G_ACTION_MAP(group), "cursor_follows_playback", "cursor_follows_playback");
    hookup_action_to_menu_item(G_ACTION_MAP(group), "stop_after_current", "stop_after_current");
    hookup_action_to_menu_item(G_ACTION_MAP(group), "stop_after_album", "stop_after_album");

    hookup_action_to_radio_menu_item(G_ACTION_MAP(group), "shufflemode", G_CALLBACK(order_linear_activate), "order_linear");
    hookup_action_to_radio_menu_item(G_ACTION_MAP(group), "shufflemode", G_CALLBACK(order_shuffle_activate), "order_shuffle");
    hookup_action_to_radio_menu_item(G_ACTION_MAP(group), "shufflemode", G_CALLBACK(order_shuffle_albums_activate), "order_shuffle_albums");
    hookup_action_to_radio_menu_item(G_ACTION_MAP(group), "shufflemode", G_CALLBACK(order_random_activate), "order_random");

    hookup_action_to_radio_menu_item(G_ACTION_MAP(group), "repeatmode", G_CALLBACK(loop_disable_activate), "loop_disable");
    hookup_action_to_radio_menu_item(G_ACTION_MAP(group), "repeatmode", G_CALLBACK(loop_single_activate), "loop_single");
    hookup_action_to_radio_menu_item(G_ACTION_MAP(group), "repeatmode", G_CALLBACK(loop_all_albums_activate), "loop_all");


    g_object_set(G_OBJECT(headerbar), "spacing", headerbarui_flags.button_spacing, NULL);
    gtk_widget_show(headerbar);

    gtk_window_set_titlebar(GTK_WINDOW(mainwin), GTK_WIDGET(headerbar));

    if (!headerbarui_flags.embed_menubar)
    {
        gtk_widget_hide(menubar);
        deadbeef->conf_set_int ("gtkui.show_menu", 0);

        headerbarui_update_menubutton();

        gtk_widget_set_can_focus(headerbar_menubtn, FALSE);
        gtk_widget_show (headerbar_menubtn);
    } else {
        gtk_widget_destroy(headerbar_menubtn);
        gtk_widget_destroy(headerbar_app_menu_btn);
        gtk_widget_destroy(GTK_WIDGET(headerbar_add_menu_btn));
        gtk_widget_set_valign(menubar, GTK_ALIGN_CENTER);
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        gtk_widget_reparent(menubar, headerbar);
#pragma GCC diagnostic pop
        gtk_container_child_set(GTK_CONTAINER(headerbar), menubar, "position", 0, NULL);
    }


    if (!headerbarui_flags.combined_playpause) {
        gtk_widget_show(headerbar_playbtn);
        gtk_widget_show(headerbar_pausebtn);
    }

    gtk_widget_set_visible(headerbar_prefsbtn, headerbarui_flags.show_preferences_button);

    gtk_widget_set_visible(headerbar_add_menu_btn, headerbarui_flags.show_add_button);

    float volume = deadbeef->volume_get_amp();
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (volbutton), volume*volume*volume);

    gtk_widget_show(volbutton);

    gtk_builder_add_callback_symbols(builder,
        "on_volbutton_value_changed", (GCallback)on_volbutton_value_changed,
        "on_seekbar_button_press_event", on_seekbar_button_press_event,
        "on_seekbar_button_release_event", on_seekbar_button_release_event,
        "on_seekbar_value_changed", on_seekbar_value_changed,
        "on_durationlabel_button_release_event", on_durationlabel_button_release_event,
        NULL);
    gtk_builder_connect_signals(builder, NULL);

    GtkWidget *titlechild = gtk_header_bar_get_custom_title (GTK_HEADER_BAR (headerbar));
    g_signal_connect (G_OBJECT(titlechild),
        "size-allocate",
        G_CALLBACK(on_title_child_size_allocate),
        NULL);

    g_signal_connect (G_OBJECT(mainwin),
        "notify::title",
        G_CALLBACK(mainwindow_settitle),
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
    headerbarui_flags.show_time_remaining = deadbeef->conf_get_int ("headerbarui.show_time_remaining", 0);
    headerbarui_flags.hide_playback_buttons = deadbeef->conf_get_int ("headerbarui.hide_playback_buttons", 0);
    headerbarui_flags.new_app_menu = deadbeef->conf_get_int ("headerbarui.new_app_menu", 0);
    headerbarui_flags.show_add_button = deadbeef->conf_get_int ("headerbarui.show_add_button", 0);
    headerbarui_flags.show_playback_button = deadbeef->conf_get_int ("headerbarui.show_playback_button", 0);
}

static
int headerbarui_connect() {
    headerbarui_getconfig();
    if (headerbarui_flags.disable) return 1;
    gtkui_plugin = (ddb_gtkui_t *) deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    if (gtkui_plugin) {
        if (gtkui_plugin->gui.plugin.version_major >= 2) {

            if (!deadbeef->plug_get_for_id ("hotkeys")) {
                fprintf (stderr, "Headerbar plugin failure: Hotkeys plugin is required!\n");
                return -1;
            }

            gtkui_plugin->add_window_init_hook (window_init_hook, NULL);
            return 0;
        } else {
            fprintf (stderr, "Headerbar plugin failure: DeaDBeeF version 1.7 or higher is required!\n");
        }
    }
    return -1;
}

static
gboolean
headerbarui_volume_changed(gpointer user_data)
{
    float volume = cbrt(deadbeef->volume_get_amp());

    GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC);
    g_signal_handlers_block_matched ((gpointer)volbutton, mask, 0, 0, NULL, on_volbutton_value_changed, NULL);
    gtk_scale_button_set_value( GTK_SCALE_BUTTON (volbutton), volume);
    g_signal_handlers_unblock_matched ((gpointer)volbutton, mask, 0, 0, NULL, on_volbutton_value_changed, NULL);
    return FALSE;
}


void
playpause_update(int state) {
    if (headerbarui_flags.hide_playback_buttons) return;

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

static int actions_changed_source;
static int config_changed_source;

static
gboolean
headerbarui_configchanged_cb(gpointer user_data)
{
    gtk_widget_set_visible(headerbar_seekbarbox, headerbarui_flags.show_seek_bar && !headerbarui_flags.seekbar_minimized && seekbar_isvisible);
    gtk_widget_set_visible(headerbar_stopbtn, headerbarui_flags.show_stop_button);
    gtk_widget_set_visible(volbutton, headerbarui_flags.show_volume_button);
    gtk_widget_set_visible(headerbar_prefsbtn, headerbarui_flags.show_preferences_button);
    gtk_widget_set_visible(headerbar_prefsbtn, headerbarui_flags.show_preferences_button);
    gtk_widget_set_visible(headerbar_designmodebtn, headerbarui_flags.show_designmode_button);
    g_object_set(G_OBJECT(headerbar), "spacing", headerbarui_flags.button_spacing, NULL);
    playpause_update(OUTPUT_STATE_STOPPED);

    gtk_widget_set_visible(headerbar_playback_button_box, !headerbarui_flags.hide_playback_buttons);

    gtk_widget_set_visible(headerbar_app_menu_btn, headerbarui_flags.new_app_menu);
    gtk_widget_set_visible(headerbar_menubtn, !headerbarui_flags.new_app_menu);

    gtk_widget_set_visible(headerbar_add_menu_btn, headerbarui_flags.show_add_button);
    gtk_widget_set_visible(headerbar_playback_menu_btn, headerbarui_flags.show_playback_button);

    // Only regenerate the menus if required
    if (headerbarui_flags.show_playback_button_prev != headerbarui_flags.show_playback_button) {
        update_plugin_actions();
        headerbarui_flags.show_playback_button_prev = headerbarui_flags.show_playback_button;
    }

    config_changed_source = 0;
    return FALSE;
}

static
gboolean
headerbarui_actionschanged_cb(gpointer user_data)
{
    update_plugin_actions();
    actions_changed_source = 0;
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
    case DB_EV_ACTIONSCHANGED:
        if (actions_changed_source) {
            g_source_remove(actions_changed_source);
        }
        actions_changed_source = g_idle_add (headerbarui_actionschanged_cb, NULL);
        break;
    case DB_EV_CONFIGCHANGED:
        headerbarui_getconfig();
        if (config_changed_source) {
            g_source_remove(config_changed_source);
        }
        config_changed_source = g_idle_add (headerbarui_configchanged_cb, NULL);
        g_idle_add (headerbarui_volume_changed, NULL);
        break;
    case DB_EV_VOLUMECHANGED:
        g_idle_add (headerbarui_volume_changed, NULL);
        break;
    case DB_EV_TERMINATE:
        if (actions_changed_source) {
            g_source_remove(actions_changed_source);
        }
        if (config_changed_source) {
            g_source_remove(config_changed_source);
        }
        break;
    }
    return 0;
}

int headerbarui_disconnect(void)
{
    // HACK, need to reset this so users are not stuck without menubar when uninstalling this plugin
    deadbeef->conf_set_int ("gtkui.show_menu", 1);

    return 0;
}

static const char settings_dlg[] =
    "property \"Disable plugin (requires restart)\" checkbox headerbarui.disable 0;\n"
    "property \"Embed menubar instead of showing hamburger button (requires restart)\" checkbox headerbarui.embed_menubar 0;\n"
    "property \"Show new popover menu instead of traditional\ncontext menu on hamburger button\" checkbox headerbarui.new_app_menu 0;\n"
    "property \"Button spacing (pixels)\" spinbtn[0,100,1] headerbarui.button_spacing 6;\n"
    "property \"Use combined play/pause button\" checkbox headerbarui.combined_playpause 1;\n"
    "property \"Show seekbar\" checkbox headerbarui.show_seek_bar 1;\n"
    "property \"Hide seekbar on streaming\" checkbox headerbarui.hide_seekbar_on_streaming 0;\n"
    "property \"Show stop button\" checkbox headerbarui.show_stop_button 1;\n"
    "property \"Show volume button\" checkbox headerbarui.show_volume_button 1;\n"
    "property \"Show preferences button\" checkbox headerbarui.show_preferences_button 0;\n"
    "property \"Show design mode button\" checkbox headerbarui.show_designmode_button 0;\n"
    "property \"Hide playback buttons\" checkbox headerbarui.hide_playback_buttons 0;\n"
    "property \"Show add (file/playlist) button\" checkbox headerbarui.show_add_button 0;\n"
    "property \"Show playback options (shuffle/repeat) button\" checkbox headerbarui.show_playback_button 0;\n"
;

static DB_misc_t plugin = {
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 9, // using 10 apis if available
#if (DDB_API_LEVEL >= 10)
    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
#endif
    .plugin.version_major = 1,
    .plugin.version_minor = 1,
    .plugin.id = "headerbarui_gtk3",
    .plugin.name = "Headerbar for GTK3 UI",
    .plugin.descr = "A headerbar for the GTK3 UI",
    .plugin.copyright = 
        "Headerbar for GTK3 UI plugin for DeaDBeeF Player\n"
        "Copyright (C) 2015-2021 Nicolai Syvertsen <saivert@gmail.com>\n"
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
    .plugin.website = "https://github.com/saivert/ddb_misc_headerbar_GTK3",
    .plugin.configdialog = settings_dlg,
    .plugin.connect = headerbarui_connect,
    .plugin.message = headerbarui_message,
    .plugin.disconnect = headerbarui_disconnect
};

DB_plugin_t *
ddb_misc_headerbar_GTK3_load (DB_functions_t *api) {
    g_resources_register(headerbarui_get_resource());
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
