#include <gtk/gtk.h>
#include "ddbheaderbar.h"
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include "headerbarui.h"

static void
ddb_header_bar_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (DdbHeaderBar, ddb_header_bar, GTK_TYPE_HEADER_BAR,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                         ddb_header_bar_buildable_init))


static gint
seekbar_width (int mainwin_width) {
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

void
ddb_header_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{

    GtkWidget *headerbar_seekbar = DDB_HEADER_BAR(widget)->seekbar;
    if (headerbarui_flags.show_seek_bar && seekbar_isvisible) {

        int width = seekbar_width(allocation->width);

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

    GTK_WIDGET_CLASS (ddb_header_bar_parent_class)->size_allocate(widget, allocation);
}

GtkWidget *
ddb_header_bar_new (void)
{
    return GTK_WIDGET (g_object_new (DDB_TYPE_HEADER_BAR, NULL));
}

static void
ddb_header_bar_class_init (DdbHeaderBarClass *class)
{
    // GObjectClass *object_class = G_OBJECT_CLASS (class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

    widget_class->size_allocate = ddb_header_bar_size_allocate;

    /* Setup the template GtkBuilder xml for this class */
    gtk_widget_class_set_template_from_resource (widget_class, "/org/deadbeef/headerbarui/headerbar.ui");

    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, volbutton);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, seekbar);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, playbtn);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, pausebtn);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, stopbtn);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, menubtn);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, prefsbtn);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, designmodebtn);

    gtk_widget_class_bind_template_callback (widget_class, on_volbutton_value_changed);
    gtk_widget_class_bind_template_callback (widget_class, on_seekbar_format_value);
    gtk_widget_class_bind_template_callback (widget_class, on_seekbar_button_press_event);
    gtk_widget_class_bind_template_callback (widget_class, on_seekbar_button_release_event);
    gtk_widget_class_bind_template_callback (widget_class, on_seekbar_value_changed);

}

static void
ddb_header_bar_init (DdbHeaderBar *bar)
{
    gtk_widget_init_template (GTK_WIDGET (bar));
}

static void
ddb_header_bar_buildable_init (GtkBuildableIface *iface)
{
}
