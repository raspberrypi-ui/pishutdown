/*============================================================================
Copyright (c) 2016-2025 Raspberry Pi
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
============================================================================*/

#include <string.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

//#define USE_LOGIND
#ifdef USE_LOGIND
GDBusProxy *proxy;
#endif

static void button_handler (GtkWidget *widget, gpointer data)
{
    if (!strcmp (data, "shutdown")) system ("/usr/bin/pkill orca;/sbin/shutdown -h now");
    if (!strcmp (data, "reboot")) system ("/usr/bin/pkill orca;/sbin/reboot");
    if (!strcmp (data, "lock"))
    {
        system ("/usr/bin/swaylock -p");
        gtk_main_quit ();
    }
    if (!strcmp (data, "exit"))
    {
        system ("/usr/bin/pkill orca");
#ifdef USE_LOGIND
        if (proxy && !system ("systemctl is-active lightdm | grep -qw active"))
        {
            GVariant *var = g_variant_new ("(ui)", getuid(), SIGKILL);
            g_dbus_proxy_call_sync (proxy, "KillUser", var, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
        }
        else
#endif
        if (!system ("pgrep wayfire > /dev/null")) system ("/usr/bin/pkill wayfire");
        else if (!system ("pgrep labwc > /dev/null")) system ("/usr/bin/labwc -e");
        else system ("/usr/bin/pkill lxsession");
    }
}

static gboolean delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_main_quit ();
    return FALSE;
}

static gboolean key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        gtk_main_quit ();
        return TRUE;
    }
    return FALSE;
}

#ifdef USE_LOGIND
static void cb_name_owned (GDBusConnection *connection, const gchar *name, const gchar *owner, gpointer user_data)
{
    proxy = g_dbus_proxy_new_sync (connection, 0, NULL, name, "/org/freedesktop/login1", "org.freedesktop.login1.Manager", NULL, NULL);
}

static void cb_name_unowned (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    if (proxy) g_object_unref (proxy);
    proxy = NULL;
}
#endif

/* The dialog... */

int main (int argc, char *argv[])
{
    GtkWidget *dlg, *btn;
    GtkBuilder *builder;

    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    // GTK setup
    gtk_init (&argc, &argv);
    gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default(), PACKAGE_DATA_DIR);

    // build the UI
    builder = gtk_builder_new_from_file (PACKAGE_UI_DIR "/pishutdown.ui");

    dlg = (GtkWidget *) gtk_builder_get_object (builder, "main_window");
    g_signal_connect (G_OBJECT (dlg), "delete_event", G_CALLBACK (delete_event), NULL);
    g_signal_connect (G_OBJECT (dlg), "key-press-event", G_CALLBACK (key_press_event), NULL);
    gtk_widget_add_events (dlg, GDK_KEY_PRESS_MASK);

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_shutdown");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "shutdown");

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_reboot");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "reboot");

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_lock");
    if (getenv ("WAYLAND_DISPLAY") && !system ("passwd -S $USER | grep -qw P"))
        g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "lock");
    else gtk_widget_hide (btn);

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_logout");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "exit");
    if (system ("systemctl is-active lightdm | grep -qw active"))
        gtk_button_set_label (GTK_BUTTON (btn), _("Exit to command line"));

#ifdef USE_LOGIND
    // set up callbacks to find DBus interface to system-logind
    proxy = NULL;
    g_bus_watch_name (G_BUS_TYPE_SYSTEM, "org.freedesktop.login1", 0, cb_name_owned, cb_name_unowned, NULL, NULL);
#endif

    gtk_widget_show (dlg);
    gtk_main ();
    gtk_widget_destroy (dlg);

    return 0;
}

/* End of file */
/*----------------------------------------------------------------------------*/
