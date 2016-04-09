#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#define MIN_WIDTH 250

/* Get the background pixbuf. */
static GdkPixbuf * get_background_pixbuf(void) {
    /* Get the root window pixmap. */
    GdkScreen * screen = gdk_screen_get_default();
    GdkPixbuf * pixbuf = gdk_pixbuf_get_from_window(
        gdk_get_default_root_window(),
        0,
        0,
        gdk_screen_get_width(screen),		/* Width */
        gdk_screen_get_height(screen));		/* Height */

    /* Make the background darker. */
    if (pixbuf != NULL) {
        unsigned char * pixels = gdk_pixbuf_get_pixels(pixbuf);
        int width = gdk_pixbuf_get_width(pixbuf);
        int height = gdk_pixbuf_get_height(pixbuf);
        int pixel_stride = ((gdk_pixbuf_get_has_alpha(pixbuf)) ? 4 : 3);
        int row_stride = gdk_pixbuf_get_rowstride(pixbuf);
        int y;
        for (y = 0; y < height; y += 1) {
            unsigned char * p = pixels;
            int x;
            for (x = 0; x < width; x += 1)
            {
                p[0] = p[0] / 2.5;
                p[1] = p[1] / 2.5;
                p[2] = p[2] / 2.5;
                p += pixel_stride;
            }
            pixels += row_stride;
        }
    }
    return pixbuf;
}

/* Handler for "expose_event" on background. */
gboolean draw(GtkWidget * widget, cairo_t * cr, GdkPixbuf * pixbuf) {
    if (pixbuf != NULL) {
        /* Copy the appropriate rectangle of the root window pixmap to the drawing area.
         * All drawing areas are immediate children of the toplevel window, so the allocation yields the source coordinates directly. */
        gdk_cairo_set_source_pixbuf (cr,  pixbuf, 0, 0);
        cairo_paint (cr);
    }
    return FALSE;
}

static void get_string (char *cmd, char *name) {
    FILE *fp = popen (cmd, "r");
    char buf[128];

    name[0] = 0;
    if (fp == NULL) return;
    if (fgets (buf, sizeof (buf) - 1, fp) != NULL)
    {
        sscanf (buf, "%s", name);
        return;
    }
}

void button_handler (GtkWidget *widget, gpointer data) {
    if (!strcmp (data, "shutdown")) system ("sudo shutdown -h now");
    if (!strcmp (data, "reboot")) system ("sudo reboot");
    if (!strcmp (data, "exit")) system ("sudo pkill lxsession");
}

/* The dialog... */

int main (int argc, char *argv[]) {
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
	g_object_set (gtk_settings_get_default (), "gtk-application-prefer-dark-theme", TRUE, NULL);

	// build the UI
	/* Get the background pixbuf. */
	GdkPixbuf * pixbuf = get_background_pixbuf();

	/* Create the toplevel window. */
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_fullscreen(GTK_WINDOW(window));
	GdkScreen* screen = gtk_widget_get_screen(window);
	gtk_window_set_default_size(GTK_WINDOW(window), gdk_screen_get_width(screen), gdk_screen_get_height(screen));
	gtk_widget_set_app_paintable(window, TRUE);
	g_signal_connect(G_OBJECT(window), "draw", G_CALLBACK(draw), pixbuf);

	/* Toplevel container */
	GtkWidget* alignment = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign (alignment, GTK_ALIGN_CENTER);
	gtk_widget_set_valign (alignment, GTK_ALIGN_CENTER);
	gtk_container_add(GTK_CONTAINER(window), alignment);

	GtkWidget* center_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_add(GTK_CONTAINER(alignment), center_area);

	/* Create the label. */
	GtkWidget * label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label), _("<b><big>Shutdown your Raspberry Pi</big></b>"));
	gtk_box_pack_start(GTK_BOX(center_area), label, FALSE, FALSE, 4);

	GtkWidget* center_vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_set_border_width(GTK_CONTAINER(center_vbox), 12);
	gtk_container_add(GTK_CONTAINER(center_area), center_vbox);

	GtkWidget* controls = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_size_request (controls, 250, -1);
	gtk_widget_set_halign (controls, GTK_ALIGN_CENTER);
	gtk_widget_set_valign (controls, GTK_ALIGN_CENTER);

	GtkWidget * banner_image;
	
	GError *error = NULL;
	GdkPixbuf *image = gdk_pixbuf_new_from_file_at_size("/usr/share/raspberrypi-artwork/raspberry-pi-logo.svg", 190, 190, &error);
	if(!image) {
		g_print("Error: %s\n", error->message);
		g_error_free(error);
		banner_image = gtk_image_new_from_icon_name("system-shutdown", 0);
		gtk_image_set_pixel_size (GTK_IMAGE (banner_image), 180);
	} else {
		banner_image = gtk_image_new_from_pixbuf(image);
		g_object_unref(image);
	}
	
	gtk_box_pack_start(GTK_BOX(center_vbox), banner_image, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(center_vbox), controls, FALSE, FALSE, 2);

	/* Shutdown */
	GtkWidget * shutdown_button = gtk_button_new();
	GtkWidget * sdlabel = gtk_label_new_with_mnemonic (_("_Shutdown"));
	gtk_widget_set_halign (sdlabel, GTK_ALIGN_START);
	gtk_container_add(GTK_CONTAINER (shutdown_button), sdlabel);
	g_signal_connect(G_OBJECT(shutdown_button), "clicked", G_CALLBACK (button_handler), "shutdown");
	gtk_box_pack_start(GTK_BOX(controls), shutdown_button, FALSE, FALSE, 4);
	gtk_style_context_add_class(gtk_widget_get_style_context(shutdown_button), "suggested-action");

	/* Reboot */
	GtkWidget * reboot_button = gtk_button_new();
	GtkWidget * rblabel = gtk_label_new_with_mnemonic (_("_Reboot"));
	gtk_widget_set_halign (rblabel, GTK_ALIGN_START);
	gtk_container_add(GTK_CONTAINER (reboot_button), rblabel);
	g_signal_connect(G_OBJECT(reboot_button), "clicked", G_CALLBACK (button_handler), "reboot");
	gtk_box_pack_start(GTK_BOX(controls), reboot_button, FALSE, FALSE, 4);

	/* Logout */
	GtkWidget * logout_button = gtk_button_new();
	GtkWidget * lolabel;
	get_string ("service lightdm status | grep \"\\bactive\\b\"", buffer);
	if (strlen (buffer))
		lolabel = gtk_label_new_with_mnemonic (_("_Logout"));
	else
		lolabel = gtk_label_new_with_mnemonic (_("Return to Command _Line"));

	gtk_widget_set_halign (lolabel, GTK_ALIGN_START);
	gtk_container_add(GTK_CONTAINER (logout_button), lolabel);
	g_signal_connect(G_OBJECT(logout_button), "clicked", G_CALLBACK (button_handler), "exit");
	gtk_box_pack_start(GTK_BOX(controls), logout_button, FALSE, FALSE, 4);

	/* Create the Cancel button. */
	GtkWidget * cancel_button = gtk_button_new();
	GtkWidget * cnlabel = gtk_label_new_with_mnemonic (_("_Cancel"));
	gtk_widget_set_halign (cnlabel, GTK_ALIGN_START);
	gtk_container_add(GTK_CONTAINER (cancel_button), cnlabel);
	g_signal_connect(G_OBJECT(cancel_button), "clicked", G_CALLBACK(gtk_main_quit), NULL);
	
	GtkAccelGroup* accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	gtk_widget_add_accelerator(cancel_button, "activate", accel_group, GDK_KEY_Escape, (GdkModifierType)0, GTK_ACCEL_VISIBLE);
	gtk_box_pack_start(GTK_BOX(controls), cancel_button, FALSE, FALSE, 4);
	gtk_widget_show_all(window);

	gtk_main ();

	return 0;
}
