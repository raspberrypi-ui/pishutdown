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
#include <libxml/xpathInternals.h>

#include "activate.h"

/*----------------------------------------------------------------------------*/
/* Typedefs and macros                                                        */
/*----------------------------------------------------------------------------*/

#define XC(str) ((xmlChar *) str)

/*----------------------------------------------------------------------------*/
/* Global data                                                                */
/*----------------------------------------------------------------------------*/

char *lockbind = NULL;

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static void read_xml (const char *file);
static char *expand_keystring (const char *in);
static char *decamel (const char *in);
static void button_handler (GtkWidget *widget, gpointer data);
static gboolean delete_event (GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer data);

/*----------------------------------------------------------------------------*/
/* Function definitions                                                       */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Read lock screen key binding                                               */
/*----------------------------------------------------------------------------*/

static void read_xml (const char *file)
{
    xmlDocPtr xDoc;
    xmlXPathObjectPtr xpathObj, xpathObj2, xpathObj3;
    xmlXPathContextPtr xpathCtx;
    xmlNode *node;
    xmlAttr *attr, *attr2;
    char *key, *act, *name, *param;
    int i, j;

    // read in data from XML file
    xmlInitParser ();
    LIBXML_TEST_VERSION
    xDoc = xmlReadFile (file, NULL, XML_PARSE_NOBLANKS);
    if (xDoc == NULL)
    {
        xmlCleanupParser ();
        return;
    }

    xpathCtx = xmlXPathNewContext (xDoc);
    xmlXPathRegisterNs (xpathCtx, XC ("o"), XC ("http://openbox.org/3.4/rc"));

    xpathObj = xmlXPathEvalExpression (XC ("/o:openbox_config/o:keyboard/o:keybind"), xpathCtx);
    if (!xmlXPathNodeSetIsEmpty (xpathObj->nodesetval))
    {
        for (i = 0; i < xpathObj->nodesetval->nodeNr; i++)
        {
            key = NULL;
            act = NULL;
            name = NULL;
            param = NULL;

            node = xpathObj->nodesetval->nodeTab[i];
            for (attr = node->properties; attr; attr = attr->next)
            {
                if (!attr->children || !attr->children->content) continue;
                if (!xmlStrcmp (attr->name, XC ("key")))
                {
                    key = g_strdup ((char *) attr->children->content);
                    if (lockbind && !g_strcmp0 (key, lockbind))
                    {
                        g_free (lockbind);
                        lockbind = NULL;
                    }
                }
            }
            xpathObj2 = xmlXPathNodeEval (node, XC ("./o:action"), xpathCtx);
            if (!xmlXPathNodeSetIsEmpty (xpathObj2->nodesetval))
            {
                for (attr2 = xpathObj2->nodesetval->nodeTab[0]->properties; attr2; attr2 = attr2->next)
                {
                    if (!attr2->children || !attr2->children->content) continue;
                    if (!xmlStrcmp (attr2->name, XC ("name")))
                        act = g_strdup ((char *) attr2->children->content);
                    if (!xmlStrcmp (attr2->name, XC ("command")))
                    {
                        name = g_strdup ((char *) attr2->name);
                        param = g_strdup ((char *) attr2->children->content);
                    }
                }

                node = xpathObj2->nodesetval->nodeTab[0];
                xpathObj3 = xmlXPathNodeEval (node, XC ("./o:*"), xpathCtx);
                if (!xmlXPathNodeSetIsEmpty (xpathObj3->nodesetval))
                {
                    for (j = 0; j < xpathObj3->nodesetval->nodeNr; j++)
                    {
                        node = xpathObj3->nodesetval->nodeTab[j];
                        if (act == NULL && !xmlStrcmp (node->name, XC ("name")))
                            act = g_strdup ((char *) xmlNodeGetContent (node));
                        if (name == NULL && !xmlStrcmp (node->name, XC ("command")))
                        {
                            name = g_strdup ((char *) node->name);
                            param = g_strdup ((char *) xmlNodeGetContent (node));
                        }
                    }
                }
                xmlXPathFreeObject (xpathObj3);
            }
            xmlXPathFreeObject (xpathObj2);

            if (!g_strcmp0 (act, "Execute") && !g_strcmp0 (name, "command") && !g_strcmp0 (param, "swaylock -p"))
                lockbind = g_strdup (key);

            g_free (key);
            g_free (act);
            g_free (name);
            g_free (param);
        }
    }
    xmlXPathFreeObject (xpathObj);

    // cleanup XML
    xmlXPathFreeContext (xpathCtx);
    xmlFreeDoc (xDoc);
    xmlCleanupParser ();
}

static char *expand_keystring (const char *in)
{
    char buf[128], *optr = buf, *iptr = (char *) in;

    while (*iptr)
    {
        if (*(iptr + 1) == '-')
        {
            switch (*iptr)
            {
                case 'S' :  sprintf (optr, "Shift-");
                            optr += 6;
                            iptr += 2;
                            break;
                case 'C' :  sprintf (optr, "Ctrl-");
                            optr += 5;
                            iptr += 2;
                            break;
                case 'A' :  sprintf (optr, "Alt-");
                            optr += 4;
                            iptr += 2;
                            break;
                case 'H' :  sprintf (optr, "Hyper-");
                            optr += 6;
                            iptr += 2;
                            break;
                case 'W' :  sprintf (optr, "Win-");
                            optr += 4;
                            iptr += 2;
                            break;
                case 'M' :  sprintf (optr, "Meta-");
                            optr += 5;
                            iptr += 2;
                            break;
                default :   *optr++ = *iptr++;
                            break;
            }
        }
        else *optr++ = *iptr++;
    }
    *optr = 0;

    iptr = strstr (buf, "XF86");
    if (iptr)
    {
        optr = decamel (iptr + 4);
        strcpy (iptr, optr);
        g_free (optr);
    }

    // xkb reports 'space', but the default config includes 'Space'...
    iptr = strstr (buf, "Space");
    if (iptr) *iptr = 's';

    return g_strdup (buf);
}

static char *decamel (const char *in)
{
    char *out = NULL, *tmp;

    while (*in)
    {
        tmp = out;
        if (tmp == NULL)
            out = g_strdup_printf ("%c", *in);
        else if (*in >= 'A' && *in <= 'Z')
            out = g_strdup_printf ("%s %c", tmp, *in);
        else
            out = g_strdup_printf ("%s%c", tmp, *in);
        g_free (tmp);
        in++;
    }

    return out;
}

/*----------------------------------------------------------------------------*/
/* Handlers                                                                   */
/*----------------------------------------------------------------------------*/

static void button_handler (GtkWidget *widget, gpointer data)
{
    if (!strcmp (data, "shutdown")) system ("/usr/bin/pkill orca;/sbin/shutdown -h now");
    if (!strcmp (data, "reboot")) system ("/usr/bin/pkill orca;/sbin/reboot");
    if (!strcmp (data, "exit"))
    {
        system ("/usr/bin/pkill orca");
        if (!system ("pgrep labwc > /dev/null")) system ("/usr/bin/labwc -e");
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

/*----------------------------------------------------------------------------*/
/* Main function                                                              */
/*----------------------------------------------------------------------------*/

int main (int argc, char *argv[])
{
    GtkWidget *dlg, *btn;
    GtkBuilder *builder;
    char *str, *lbe;

    init_dbus ("pishutdown");

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

    btn = (GtkWidget *) gtk_builder_get_object (builder, "btn_logout");
    g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (button_handler), "exit");
    if (system ("systemctl is-active lightdm | grep -qw active"))
        gtk_button_set_label (GTK_BUTTON (btn), _("Exit to command line"));

    read_xml ("/etc/xdg/labwc/rc.xml");
    str = g_build_filename (g_get_user_config_dir (), "labwc/rc.xml", NULL);
    read_xml (str);
    g_free (str);

    btn = (GtkWidget *) gtk_builder_get_object (builder, "lbl_lock");
    if (!lockbind) gtk_widget_hide (btn);
    else
    {
        lbe = expand_keystring (lockbind);
        str = g_strdup_printf (_("Press '%s' to lock screen"), lbe);
        gtk_label_set_text (GTK_LABEL (btn), str);
        g_free (str);
        g_free (lbe);
        g_free (lockbind);
    }

    setup_activate (dlg);

    gtk_widget_show (dlg);
    gtk_main ();
    gtk_widget_destroy (dlg);

    close_dbus ();

    return 0;
}

/* End of file */
/*----------------------------------------------------------------------------*/
