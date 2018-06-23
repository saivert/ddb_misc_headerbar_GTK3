#pragma once

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
    GtkAdjustment *seekbar_adjustment;
};

struct _DdbHeaderBarClass
{
  GtkHeaderBarClass parent_class;

};

GtkWidget *
ddb_header_bar_new (void);

G_END_DECLS
