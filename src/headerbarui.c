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
#include "ddbheaderbar.h"

DB_functions_t *deadbeef;
static DB_misc_t plugin;

ddb_gtkui_t *gtkui_plugin;

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

#define GTK_BUILDER_GET_WIDGET(builder, name) (GtkWidget *)gtk_builder_get_object(builder, name)

struct headerbarui_flag_s headerbarui_flags;

// Just a test of a second instance
#ifdef HB2
GtkWidget *hb2;
#endif


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

gchar *
format_time(gdouble value)
{
    int time = value;
    int hr = time/3600;
    int mn = (time-hr*3600)/60;
    int sc = time-hr*3600-mn*60;
    gchar *text;
    if (hr==0)
        text = g_strdup_printf ("%02d:%02d", mn, sc);
    else
        text = g_strdup_printf ("%02d:%02d:%02d", hr, mn, sc);

    return text;
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
            if (dbaction->callback2 && dbaction->flags & DB_ACTION_COMMON) {
                action = g_simple_action_new (dbaction->name, NULL);
                g_object_set_data (G_OBJECT (action), "deadbeefaction", dbaction);
                g_signal_connect (action, "activate", action_activate, NULL);
                g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));
            }

        }
    }

    return G_ACTION_GROUP (group);
}

void window_init_hook (void *userdata) {
    GtkWidget *menubar;
    GtkBuilder *builder;

    mainwin = gtkui_plugin->get_mainwin ();

    menubar = lookup_widget (GTK_WIDGET(mainwin), "menubar");
    g_assert_nonnull(mainwin);
    g_assert_nonnull(menubar);

    //builder = gtk_builder_new();
    //gtk_builder_add_from_resource (builder, "/org/deadbeef/headerbarui/menu.ui", NULL);
    headerbar = ddb_header_bar_new();
    gtk_widget_show(headerbar);
    gtk_window_set_titlebar(GTK_WINDOW (mainwin), GTK_WIDGET(headerbar));


    //GMenuModel *menumodel = G_MENU_MODEL (gtk_builder_get_object (builder, "file-menu"));

    //GtkWidget *file_menu_btn = GTK_MENU_BUTTON (gtk_builder_get_object (builder, "file_menu_btn"));
    //gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (file_menu_btn), menumodel);

    GActionGroup *group = create_action_group();
    gtk_widget_insert_action_group (mainwin, "win", group);

    GAction *designmode_action = g_action_map_lookup_action (G_ACTION_MAP (group), "designmode");

    g_signal_connect_after (G_OBJECT (lookup_widget (mainwin, "design_mode1")),
        "activate", G_CALLBACK (design_mode_menu_item_activate), designmode_action);

    GActionGroup *deadbeef_action_group = create_action_group_deadbeef();    
    gtk_widget_insert_action_group (mainwin, "db", deadbeef_action_group);



#ifdef HB2
    // Just a test of a second instance
    hb2 = ddb_header_bar_new();
    gtk_container_add_with_properties(lookup_widget(mainwin, "vbox1"), hb2, "expand", FALSE, NULL);
    gtk_header_bar_set_decoration_layout (hb2, "");
    DDB_HEADER_BAR(hb2)->is_statusbar=1;
#endif
}


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

gboolean headebarui_delayed_startup(gpointer user_data)
{
    if (headerbar) ddb_header_bar_message(headerbar, DB_EV_SONGSTARTED, 0, 0, 0);
    return FALSE;
}

static int
headerbarui_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {

#ifdef HB2
    if (hb2) {
        ddb_header_bar_message (hb2, id, ctx, p1, p2);
    }
#endif

    if (headerbar) {
        return ddb_header_bar_message(headerbar, id, ctx, p1, p2);
    }

    // This event is emitted before headerbar is initialized on deadbeef startup
    if (id == DB_EV_SONGSTARTED) {
        g_idle_add(headebarui_delayed_startup, NULL);
    }
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
