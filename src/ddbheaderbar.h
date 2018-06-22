#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DDB_TYPE_HEADER_BAR            (ddb_header_bar_get_type ())
#define DDB_HEADER_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDB_TYPE_HEADER_BAR, DdbHeaderBar))
#define DDB_HEADER_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DDB_TYPE_HEADER_BAR, DdbHeaderBarClass))
#define DDB_IS_HEADER_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_HEADER_BAR))
#define DDB_IS_HEADER_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DDB_TYPE_HEADER_BAR))
#define DDB_HEADER_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_HEADER_BAR, DdbHeaderBarClass))

typedef struct _DdbHeaderBar              DdbHeaderBar;
//typedef struct _DdbHeaderBarPrivate       DdbHeaderBarPrivate;
typedef struct _DdbHeaderBarClass         DdbHeaderBarClass;

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

G_END_DECLS
