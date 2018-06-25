#pragma once

#include <stdint.h>
#include <gtk/gtk.h>
G_BEGIN_DECLS


#define DDB_TYPE_HEADER_BAR            (ddb_header_bar_get_type ())

G_DECLARE_FINAL_TYPE(DdbHeaderBar, ddb_header_bar, DDB, HEADER_BAR, GtkHeaderBar)



struct _DdbHeaderBar
{
    GtkHeaderBar headerbar;

    GtkWidget *volbutton;
    GtkWidget *seekbar;
    GtkWidget *playbtn;
    GtkWidget *pausebtn;
    GtkWidget *stopbtn;
    GtkWidget *menubtn;
    GtkWidget *prefsbtn;
    GtkWidget *designmodebtn;
    GtkWidget *titlelabel;
    GtkWidget *durationlabel;
    GtkWidget *playbacktimelabel;
    GtkWidget *seekbarbox;
    
    gboolean seekbar_ismoving;
    gboolean seekbar_isvisible;
    gboolean stoptimer;
    guint timer;
#ifdef HB2
    gboolean is_statusbar;
#endif
};

struct _DdbHeaderBarClass
{
  GtkHeaderBarClass parent_class;

};

GtkWidget *
ddb_header_bar_new (void);

int
ddb_header_bar_message (DdbHeaderBar *bar, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);

G_END_DECLS
