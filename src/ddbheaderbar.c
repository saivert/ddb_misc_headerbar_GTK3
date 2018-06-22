#include <gtk/gtk.h>
#include "ddbheaderbar.h"
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include "headerbarui.h"

//G_DEFINE_TYPE(DdbHeaderBar, ddb_header_bar, GTK_TYPE_HEADER_BAR)

static void
ddb_header_bar_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (DdbHeaderBar, ddb_header_bar, GTK_TYPE_HEADER_BAR,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                         ddb_header_bar_buildable_init));

GtkWidget *
ddb_header_bar_new (void)
{
    return GTK_WIDGET (g_object_new (DDB_TYPE_HEADER_BAR, NULL));
}

static void
ddb_header_bar_class_init (DdbHeaderBarClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);
    DdbHeaderBarClass *us = DDB_HEADER_BAR_CLASS (class);

    /* Setup the template GtkBuilder xml for this class */
    gtk_widget_class_set_template_from_resource (widget_class, "/org/deadbeef/headerbarui/headerbar.ui");

    //gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, seekbar_adjustment);
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
