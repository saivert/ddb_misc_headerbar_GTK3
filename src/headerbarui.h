#if ENABLE_NLS

# include <libintl.h>
#define _(s) gettext(s)

#else

#define _(s) (s)

#endif

#define trace(...) { fprintf(stderr, __VA_ARGS__); }

extern DB_functions_t *deadbeef;
//extern seekbar_ismoving;

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
