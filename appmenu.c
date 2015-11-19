#include "appmenu.h"
#include "headerbarui.h"

void activate_uno(GSimpleAction *action, GVariant* a, gpointer b)
{
    gchar *strval = NULL;
    g_object_get(action, "name", &strval, NULL);
    if (!strval)
        return;

    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                     flags,
                                     GTK_MESSAGE_INFO,
                                     GTK_BUTTONS_OK,
                                     "Appmenu action called “%s”",
                                     strval);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

}

static const GActionEntry app_entries[] = {
  { "Preferences", activate_uno, NULL, NULL, NULL, {0} },
  { "About", activate_uno, NULL, NULL, NULL, {0} },
  { "Quit", activate_uno, NULL, NULL, NULL, {0} },
};

void
headerbarui_add_app_menu(GtkWindow *mainwin)
{
    GdkWindow* gdkWindow;
    GMenu *menu;
    GMenuItem* item;
    GError *error=NULL;

    GDBusConnection *pSessionBus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
    if (error != NULL)
    {
        trace("ERROR: headerbar dbus setup fail: %s", error->message);
        g_clear_error(&error);
        return;
    }
    g_assert (pSessionBus != NULL);

    gdkWindow = gtk_widget_get_window( GTK_WIDGET(mainwin) );
    if (!GDK_IS_X11_WINDOW (gdkWindow))
        return;

    gdk_x11_window_set_utf8_property( gdkWindow, "_GTK_APPLICATION_ID", "org.deadbeef" );
    gdk_x11_window_set_utf8_property( gdkWindow, "_GTK_UNIQUE_BUS_NAME", g_dbus_connection_get_unique_name( pSessionBus ) );
    gdk_x11_window_set_utf8_property( gdkWindow, "_GTK_APPLICATION_OBJECT_PATH", "/org/deadbeef" );
    Window windowId = GDK_WINDOW_XID( gdkWindow );
    gchar* aDBusWindowPath = g_strdup_printf( "/org/deadbeef/window/%lu", windowId );
    gdk_x11_window_set_utf8_property( gdkWindow, "_GTK_WINDOW_OBJECT_PATH", aDBusWindowPath );
    g_free(aDBusWindowPath);


    menu = g_menu_new ();

    item = g_menu_item_new(_("Preferences"), "app.Preferences");
    g_menu_append_item( menu, item );
    g_object_unref(item);

    item = g_menu_item_new(_("About"), "app.About");
    g_menu_append_item( menu, item );
    g_object_unref(item);

    item = g_menu_item_new(_("Quit"), "app.Quit");
    g_menu_append_item( menu, item );
    g_object_unref(item);

    GSimpleActionGroup *group = g_simple_action_group_new ();
    g_action_map_add_action_entries (G_ACTION_MAP (group), app_entries, G_N_ELEMENTS (app_entries), NULL);
    GActionGroup* pAppActionGroup = G_ACTION_GROUP(group);
    if (!g_dbus_connection_export_action_group( pSessionBus, "/org/deadbeef", pAppActionGroup, NULL))
    {
        trace("ERROR: g_dbus_connection_export_action_group call fail");
        goto errexit;
    }
    g_dbus_connection_export_action_group( pSessionBus, "/org/deadbeef", pAppActionGroup, NULL);
    g_object_unref(pAppActionGroup);
    if (!g_dbus_connection_export_menu_model (pSessionBus, "/org/deadbeef/menus/appmenu", G_MENU_MODEL (menu), NULL))
    {
        trace("ERROR: g_dbus_connection_export_menu_model call fail");
        goto errexit;
    }
    g_bus_own_name_on_connection (pSessionBus, "org.deadbeef", 0, NULL, NULL, NULL, NULL);
    errexit:
    g_object_unref(menu);

}