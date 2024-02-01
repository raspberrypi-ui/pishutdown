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
#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <linux/input.h>

#define USE_LOGIND

#ifdef USE_LOGIND
GDBusProxy *proxy;
#endif

static int open_restricted (const char *path, int flags, void *user_data)
{
    int fd = open (path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted (int fd, void *user_data)
{
    close (fd);
}

const static struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

static void button_handler (GtkWidget *widget, gpointer data)
{
    if (!strcmp (data, "shutdown")) system ("/usr/bin/pkill orca;/sbin/shutdown -h now");
    if (!strcmp (data, "reboot")) system ("/usr/bin/pkill orca;/sbin/reboot");
    if (!strcmp (data, "exit"))
    {
        system ("/usr/bin/pkill orca");
#ifdef USE_LOGIND
        if (proxy)
        {
            GVariant *var = g_variant_new ("(ui)", getuid(), SIGKILL);
            g_dbus_proxy_call_sync (proxy, "KillUser", var, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
        }
        else
#endif
        if (!system ("pgrep wayfire > /dev/null")) system ("/usr/bin/pkill wayfire");
        else if (!system ("pgrep labwc > /dev/null")) system ("/usr/bin/pkill labwc");
        else system ("/bin/kill $_LXSESSION_PID");
    }
}

static gint delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_main_quit ();
    return FALSE;
}

static gboolean check_libinput_events (struct libinput *li)
{
    struct libinput_event *ev;
    libinput_dispatch (li);
    if ((ev = libinput_get_event (li)) != 0)
    {
        if (libinput_event_get_type (ev) == LIBINPUT_EVENT_KEYBOARD_KEY)
        {
            struct libinput_event_keyboard *kb = libinput_event_get_keyboard_event (ev);
            if (libinput_event_keyboard_get_key_state (kb) == LIBINPUT_KEY_STATE_PRESSED)
            {
                switch (libinput_event_keyboard_get_key (kb))
                {
                    case KEY_POWER:
                        system ("/usr/bin/pkill orca;/sbin/shutdown -h now");
                        break;
                    case KEY_ESC:
                        gtk_main_quit ();
                        break;
                }
            }
            libinput_event_destroy (ev);
        }
    }
    return TRUE;
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

#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    // GTK setup
    gtk_init (&argc, &argv);
    gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default(), PACKAGE_DATA_DIR);

    // build the UI
    builder = gtk_builder_new_from_file (PACKAGE_UI_DIR "/pishutdown.ui");
    
    dlg = (GtkWidget *) gtk_builder_get_object (builder, "main_window");
    g_signal_connect (G_OBJECT (dlg), "delete_event", G_CALLBACK (delete_event), NULL);

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_shutdown");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "shutdown");

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_reboot");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "reboot");

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_logout");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "exit");
    if (system ("systemctl is-active lightdm | grep -qw active"))
        gtk_button_set_label (GTK_BUTTON (btn), _("Exit to command line"));

#ifdef USE_LOGIND
    // set up callbacks to find DBus interface to system-logind
    proxy = NULL;
    g_bus_watch_name (G_BUS_TYPE_SYSTEM, "org.freedesktop.login1", 0, cb_name_owned, cb_name_unowned, NULL, NULL);
#endif

    // monitor libinput for key presses
    struct udev *udev = udev_new ();
    struct libinput *li = libinput_udev_create_context (&interface, NULL, udev);
    libinput_udev_assign_seat (li, "seat0");
    libinput_dispatch (li);
    g_idle_add ((GSourceFunc) check_libinput_events, li);

    gtk_widget_show (dlg);
    gtk_main ();
    gtk_widget_destroy (dlg);

    libinput_unref (li);
    udev_unref (udev);

    return 0;
}
