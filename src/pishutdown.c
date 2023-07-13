/*
Copyright (c) 2018 Raspberry Pi (Trading) Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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

gboolean wayfire = FALSE;

static void button_handler (GtkWidget *widget, gpointer data)
{
    if (!strcmp (data, "shutdown")) system ("/usr/bin/pkill orca;/sbin/shutdown -h now");
    if (!strcmp (data, "reboot")) system ("/usr/bin/pkill orca;/sbin/reboot");
    if (!strcmp (data, "exit"))
    {
        system ("/usr/bin/pkill orca");
        if (wayfire) system ("/usr/bin/pkill wayfire");
        else system ("/bin/kill $_LXSESSION_PID");
    }
}

static gint delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_main_quit ();
    return FALSE;
}

static gboolean check_escape (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        gtk_main_quit ();
        return TRUE;
    }
    return FALSE;
}

/* The dialog... */

int main (int argc, char *argv[])
{
    GtkWidget *dlg, *btn;
    GtkBuilder *builder;

#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    if (getenv ("WAYLAND_DISPLAY")) wayfire = TRUE;

    // GTK setup
    gtk_init (&argc, &argv);
    gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default(), PACKAGE_DATA_DIR);

    // build the UI
    builder = gtk_builder_new_from_file (PACKAGE_UI_DIR "/pishutdown.ui");
    
    dlg = (GtkWidget *) gtk_builder_get_object (builder, "main_window");
    g_signal_connect (G_OBJECT (dlg), "delete_event", G_CALLBACK (delete_event), NULL);
    g_signal_connect (G_OBJECT (dlg), "key_press_event", G_CALLBACK (check_escape), NULL);

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_shutdown");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "shutdown");

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_reboot");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "reboot");

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_logout");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "exit");
    if (system ("systemctl is-active lightdm | grep -qw active"))
        gtk_button_set_label (GTK_BUTTON (btn), _("Exit to command line"));

    gtk_widget_show (dlg);
    gtk_main ();
    gtk_widget_destroy (dlg);

    return 0;
}
