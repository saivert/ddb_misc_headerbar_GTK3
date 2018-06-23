#if ENABLE_NLS

# include <libintl.h>
#define _(s) gettext(s)

#else

#define _(s) (s)

#endif

#define trace(...) { fprintf(stderr, __VA_ARGS__); }

struct headerbarui_flag_s {
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
};

extern DB_functions_t *deadbeef;
extern ddb_gtkui_t *gtkui_plugin;
extern struct headerbarui_flag_s headerbarui_flags;
extern seekbar_isvisible;

void
on_volbutton_value_changed (GtkScaleButton *button,
               gdouble         value,
               gpointer        user_data);


void
on_seekbar_value_changed (GtkRange *range,
               gpointer  user_data);

gboolean
on_seekbar_button_press_event (GtkScale *widget,
               GdkEvent  *event,
               gpointer   user_data);

gboolean
on_seekbar_button_release_event (GtkScale *widget,
               GdkEvent  *event,
               gpointer   user_data);

gchar*
on_seekbar_format_value (GtkScale *scale,
                gdouble value);
