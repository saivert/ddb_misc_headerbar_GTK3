#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>
#include "headerbarui.h"
#include "ddbheaderbar.h"

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
    int min_size_seekbar = 200;

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

    DdbHeaderBar *bar = DDB_HEADER_BAR(widget);

    if (headerbarui_flags.show_seek_bar && bar->seekbar_isvisible) {

        int width = seekbar_width(allocation->width);

        if (width == 0) {
            headerbarui_flags.seekbar_minimized = TRUE;
            gtk_widget_hide (bar->seekbarbox);
        } else {
            headerbarui_flags.seekbar_minimized = FALSE;
            gtk_widget_show (bar->seekbarbox);
        }

    }
    GTK_WIDGET_CLASS (ddb_header_bar_parent_class)->size_allocate(widget, allocation);
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

void
on_seekbar_value_changed (GtkWidget *headerbar,
               gpointer  user_data)
{
    DdbHeaderBar *bar = DDB_HEADER_BAR(headerbar);

    gdouble value = gtk_range_get_value(GTK_RANGE(bar->seekbar));
    if (DDB_HEADER_BAR(headerbar)->seekbar_ismoving) {
        gtk_label_set_text (GTK_LABEL (bar->playbacktimelabel), format_time(value));
        return;
    }
    deadbeef_seek((int)value);
}

gboolean
on_seekbar_button_press_event (GtkWidget *headerbar,
               GdkEvent  *event,
               gpointer   user_data)
{
    DdbHeaderBar *bar = DDB_HEADER_BAR(headerbar);
    bar->seekbar_ismoving = TRUE;
    return FALSE;
}

gboolean
on_seekbar_button_release_event (GtkWidget *headerbar,
               GdkEvent  *event,
               gpointer   user_data)
{
    DdbHeaderBar *bar = DDB_HEADER_BAR(headerbar);
    GtkWidget *seekbar = bar->seekbar;
    deadbeef_seek((int)gtk_range_get_value(GTK_RANGE(seekbar)));
    bar->seekbar_ismoving = FALSE;
    return FALSE;
}



void
ddb_header_bar_set_volume (DdbHeaderBar *bar, double volume)
{
    GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA);
    GQuark detail = g_quark_from_static_string("value_changed");
    g_signal_handlers_block_matched ((gpointer)bar->volbutton, mask, detail, 0, NULL, NULL, bar);
    gtk_scale_button_set_value( GTK_SCALE_BUTTON (bar->volbutton), volume );
    g_signal_handlers_unblock_matched ((gpointer)bar->volbutton, mask, detail, 0, NULL, NULL, bar);
}

void
on_volbutton_value_changed (GtkScaleButton *button,
               gdouble         value,
               gpointer        user_data)
{
        deadbeef->volume_set_db (deadbeef->volume_get_min_db()-(double)-value);
}
static
gboolean
headerbarui_volume_changed(gpointer user_data)
{
    float volume = deadbeef->volume_get_min_db()-deadbeef->volume_get_db();
    if (volume > 0) volume = 0;

    ddb_header_bar_set_volume (DDB_HEADER_BAR (user_data), -volume);
    return FALSE;
}


void
playpause_update(DdbHeaderBar *bar, int state) {
    if (headerbarui_flags.combined_playpause) {
        switch (state) {
            case OUTPUT_STATE_PLAYING:
            gtk_widget_show(bar->pausebtn);
            gtk_widget_hide(bar->playbtn);
            break;
            case OUTPUT_STATE_STOPPED:
            case OUTPUT_STATE_PAUSED:
            gtk_widget_show(bar->playbtn);
            gtk_widget_hide(bar->pausebtn);
            break;
        }
    } else {
        gtk_widget_show(bar->playbtn);
        gtk_widget_show(bar->pausebtn);
    }
}

static
void
headerbarui_adjust_seekbar_range(DdbHeaderBar *bar,
                          gdouble value,
                          gdouble lower,
                          gdouble upper,
                          gdouble step_increment,
                          gdouble page_increment,
                          gdouble page_size)
{
    GtkAdjustment * adjustment = gtk_range_get_adjustment(GTK_RANGE (bar->seekbar));

    GSignalMatchType mask = (GSignalMatchType)(G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA);
    GQuark detail = g_quark_from_static_string("value_changed");
    g_signal_handlers_block_matched ((gpointer)bar->seekbar, mask, detail, 0, NULL, NULL, bar);

    gtk_adjustment_configure(adjustment,
    value, //value
    lower, // lower
    upper, // upper
    step_increment, // step_increment
    page_increment, // page_increment
    page_size); // page_size

    gtk_label_set_text (GTK_LABEL (bar->playbacktimelabel), format_time(value));
    gtk_label_set_text (GTK_LABEL (bar->durationlabel), format_time(upper));

    g_signal_handlers_unblock_matched ((gpointer)bar->seekbar, mask, detail, 0, NULL, NULL, bar);
}

static
gboolean
headerbarui_reset_seekbar_cb(gpointer user_data)
{
    DdbHeaderBar *bar = DDB_HEADER_BAR (user_data);

    if (!headerbarui_flags.show_seek_bar) return FALSE;

    headerbarui_adjust_seekbar_range(bar,
        0, //value
        0, // lower
        0, // upper
        0, // step_increment
        0, // page_increment
        0); // page_size

    return FALSE;
}



#ifdef HB2
static
clone_statusbar_text(DdbHeaderBar *bar)
{
    GtkWidget *statusbar = lookup_widget(mainwin, "statusbar");
    if (GTK_IS_STATUSBAR(statusbar)) {
        GtkWidget *box = gtk_statusbar_get_message_area(GTK_STATUSBAR (statusbar));
        if (GTK_IS_BOX(box)) {
            GList *lst = gtk_container_get_children(GTK_CONTAINER (box));
            GtkWidget *label = GTK_WIDGET (g_list_first(lst)->data);

            if (GTK_IS_LABEL(label)) {
                gtk_header_bar_set_title (GTK_HEADER_BAR (bar), gtk_label_get_text(GTK_LABEL(label)));
            }
        }
    }
}
#endif

static
gboolean
headerbarui_update_seekbar_cb(gpointer user_data)
{
    DdbHeaderBar *bar = DDB_HEADER_BAR (user_data);
    DB_playItem_t *trk;
    DB_output_t *out;
    bar->seekbar_isvisible = TRUE;

#ifdef HB2
    if (bar->is_statusbar) {
        clone_statusbar_text(bar);
    }
#endif

    //clone_titlebar_text(bar);

    out = deadbeef->get_output();
    if (out) {
        playpause_update(bar, out->state());
        if (out->state() == OUTPUT_STATE_STOPPED) {
            bar->seekbar_isvisible = FALSE;
            goto END;
        }
    }

    if (bar->seekbar_ismoving) goto END;
    trk = deadbeef->streamer_get_playing_track ();
    if (!trk || deadbeef->pl_get_item_duration (trk) < 0) {
        if (trk) {
            deadbeef->pl_item_unref (trk);
        }
        if (headerbarui_flags.hide_seekbar_on_streaming)
            bar->seekbar_isvisible = FALSE;
        else
            headerbarui_reset_seekbar_cb(bar);
        goto END;
    }
    if (deadbeef->pl_get_item_duration (trk) > 0) {
        headerbarui_adjust_seekbar_range(bar,
            deadbeef->streamer_get_playpos (), //value
            0, // lower
            deadbeef->pl_get_item_duration (trk), // upper
            1, // step_increment
            10, // page_increment
            1); // page_size

        bar->seekbar_isvisible = TRUE;
    }
    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
END:
    if (!headerbarui_flags.seekbar_minimized) gtk_widget_set_visible(bar->seekbarbox, bar->seekbar_isvisible && headerbarui_flags.show_seek_bar);
    return !bar->stoptimer;
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
gboolean
headerbarui_configchanged_cb(gpointer user_data)
{
    DdbHeaderBar *bar = DDB_HEADER_BAR (user_data);
    gtk_widget_set_visible(bar->seekbarbox, headerbarui_flags.show_seek_bar && bar->seekbar_isvisible);
    gtk_widget_set_visible(bar->stopbtn, headerbarui_flags.show_stop_button);
    gtk_widget_set_visible(bar->volbutton, headerbarui_flags.show_volume_button);
    gtk_widget_set_visible(bar->prefsbtn, headerbarui_flags.show_preferences_button);
    gtk_widget_set_visible(bar->designmodebtn, headerbarui_flags.show_designmode_button);
    g_object_set(G_OBJECT(bar), "spacing", headerbarui_flags.button_spacing, NULL);
    playpause_update(bar, OUTPUT_STATE_STOPPED);

    return FALSE;
}

#ifdef HB2
static
gboolean
headerbarui_update_statusbartext(gpointer user_data)
{
    clone_statusbar_text(DDB_HEADER_BAR (user_data));
}
#endif

int
ddb_header_bar_message (DdbHeaderBar *bar, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2)
{
    //if (id != DB_EV_CONFIGCHANGED && headerbarui_flags.disable) return 0;
    switch (id) {
    case DB_EV_SONGSTARTED:
        bar->stoptimer = 0;
        bar->timer = g_timeout_add (1000/gtkui_get_gui_refresh_rate (), headerbarui_update_seekbar_cb, bar);
        break;
    case DB_EV_SONGFINISHED:
        bar->stoptimer = 1;
#ifdef HB2
        g_idle_add (headerbarui_update_statusbartext, bar);
#endif
        break;
    case DB_EV_CONFIGCHANGED:
        headerbarui_getconfig();
        g_idle_add (headerbarui_configchanged_cb, bar);
        g_idle_add (headerbarui_volume_changed, bar);
        break;
    case DB_EV_VOLUMECHANGED:
        g_idle_add (headerbarui_volume_changed, bar);
        break;
    }
    return 0;
}

GtkWidget *
ddb_header_bar_new (void)
{
    return GTK_WIDGET (g_object_new (DDB_TYPE_HEADER_BAR, NULL));
}

void
ddb_header_bar_set_property(GObject *object,
                            guint           property_id,
                            const GValue   *value,
                            GParamSpec     *pspec)
{
    g_debug("set propery %s", g_param_spec_get_name(pspec));

}

void
ddb_header_bar_get_property(GObject *object,
                            guint property_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    g_debug("get propery %s", g_param_spec_get_name(pspec));
}

static
void
ddb_header_bar_notify(GObject *object,
					 GParamSpec *pspec)
{
    // Since we use a custom title widget we need to reimplement copying the window title
    if (g_str_equal (g_param_spec_get_name (pspec), "title")) {
        DdbHeaderBar *bar = DDB_HEADER_BAR(object);
        gtk_label_set_text (GTK_LABEL (bar->titlelabel), gtk_header_bar_get_title (GTK_HEADER_BAR (object)));
    }
}

static void
ddb_header_bar_class_init (DdbHeaderBarClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

    object_class->set_property = ddb_header_bar_set_property;
    object_class->get_property = ddb_header_bar_get_property;
    object_class->notify = ddb_header_bar_notify;

//  GParamSpec *titlespec = g_param_spec_string ("title", "Title", "The title for the headerbar",
//                          NULL,
//                          G_PARAM_READWRITE);
//     g_object_class_install_property (object_class, 1, titlespec);

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
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, titlelabel);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, durationlabel);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, playbacktimelabel);
    gtk_widget_class_bind_template_child (widget_class, DdbHeaderBar, seekbarbox);

    gtk_widget_class_bind_template_callback (widget_class, on_volbutton_value_changed);
    gtk_widget_class_bind_template_callback (widget_class, on_seekbar_button_press_event);
    gtk_widget_class_bind_template_callback (widget_class, on_seekbar_button_release_event);
    gtk_widget_class_bind_template_callback (widget_class, on_seekbar_value_changed);

}

static void
ddb_header_bar_init (DdbHeaderBar *bar)
{
    gtk_widget_init_template (GTK_WIDGET (bar));

    if (!headerbarui_flags.combined_playpause) {
        gtk_widget_show(bar->playbtn);
        gtk_widget_show(bar->pausebtn);
    }

    gtk_widget_set_visible(bar->prefsbtn, headerbarui_flags.show_preferences_button);

    float volume = deadbeef->volume_get_min_db()-deadbeef->volume_get_db();
    g_assert_false((volume>0));
    gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON (bar->volbutton),
        gtk_adjustment_new (volume, 0, -deadbeef->volume_get_min_db (), 5, 5, 0));

    g_object_set(G_OBJECT(bar), "spacing", headerbarui_flags.button_spacing, NULL);
}

static void
ddb_header_bar_buildable_init (GtkBuildableIface *iface)
{
}
