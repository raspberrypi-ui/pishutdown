#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

#define MIN_WIDTH 275

static void get_string (char *cmd, char *name)
{
    FILE *fp = popen (cmd, "r");
    char buf[128];

    name[0] = 0;
    if (fp == NULL) return;
    if (fgets (buf, sizeof (buf) - 1, fp) != NULL)
    {
        sscanf (buf, "%s", name);
    }
    pclose (fp);
}

void button_handler (GtkWidget *widget, gpointer data)
{
    if (!strcmp (data, "shutdown")) system ("/sbin/shutdown -h now");
    if (!strcmp (data, "reboot")) system ("/sbin/reboot");
    if (!strcmp (data, "exit")) system ("/bin/kill $_LXSESSION_PID");
}

gint delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_main_quit ();
    return FALSE;
}

static gboolean check_escape (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event->keyval == GDK_KEY_Escape) {
        gtk_main_quit ();
        return TRUE;
    }
    return FALSE;
}

/* The dialog... */

int main (int argc, char *argv[])
{
    GtkWidget *dlg, *btn, *box;
    GtkRequisition req;
    char buffer[128];

#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain ( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR );
    bind_textdomain_codeset ( GETTEXT_PACKAGE, "UTF-8" );
    textdomain ( GETTEXT_PACKAGE );
#endif

    // GTK setup
    gtk_init (&argc, &argv);
    gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default(), PACKAGE_DATA_DIR);

    // build the UI
    dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Shutdown options"));
    gtk_container_set_border_width (GTK_CONTAINER (dlg), 10);
    gtk_window_set_icon (GTK_WINDOW (dlg), gdk_pixbuf_new_from_file ("/usr/share/raspberrypi-artwork/raspitr.png", NULL));
    gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
    gtk_signal_connect (GTK_OBJECT (dlg), "delete_event", G_CALLBACK (delete_event), NULL);
    gtk_signal_connect (GTK_OBJECT (dlg), "key_press_event", G_CALLBACK(check_escape), NULL);

    box = gtk_table_new (3, 1, TRUE);
    gtk_table_set_row_spacings (GTK_TABLE (box), 5);

    gtk_container_add (GTK_CONTAINER (dlg), box);

    btn = gtk_button_new_with_mnemonic (_("Shutdown"));
    gtk_widget_size_request (btn, &req);
    if (req.width < MIN_WIDTH) gtk_widget_set_size_request (box, MIN_WIDTH, -1);
    gtk_signal_connect (GTK_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "shutdown");
    gtk_table_attach_defaults (GTK_TABLE (box), btn, 0, 1, 0, 1);

    btn = gtk_button_new_with_mnemonic (_("Reboot"));
    gtk_widget_size_request (btn, &req);
    if (req.width < MIN_WIDTH) gtk_widget_set_size_request (box, MIN_WIDTH, -1);
    gtk_signal_connect (GTK_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "reboot");
    gtk_table_attach_defaults (GTK_TABLE (box), btn, 0, 1, 1, 2);

    get_string ("/usr/sbin/service lightdm status | grep \"\\bactive\\b\"", buffer);
    if (strlen (buffer))
        btn = gtk_button_new_with_mnemonic (_("Logout"));
    else
        btn = gtk_button_new_with_mnemonic (_("Exit to command line"));
    gtk_widget_size_request (btn, &req);
    if (req.width < MIN_WIDTH) gtk_widget_set_size_request (box, MIN_WIDTH, -1);
    gtk_signal_connect (GTK_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "exit");
    gtk_table_attach_defaults (GTK_TABLE (box), btn, 0, 1, 2, 3);

    gtk_window_set_position (GTK_WINDOW(dlg), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_widget_show_all (dlg);

    gtk_main ();

    return 0;
}
