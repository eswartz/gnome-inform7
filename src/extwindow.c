/*  Copyright 2006 P.F. Chimento
 *  This file is part of GNOME Inform 7.
 * 
 *  GNOME Inform 7 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  GNOME Inform 7 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNOME Inform 7; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <gnome.h>
#include <gtkspell/gtkspell.h>
#include <gtksourceview/gtksourceview.h>

#include "extwindow.h"
#include "extension.h"
#include "appwindow.h"
#include "story.h"
#include "findreplace.h"
#include "interface.h"
#include "support.h"
#include "file.h"
#include "windowlist.h"
#include "prefs.h"
#include "error.h"
#include "configfile.h"

/* Callbacks for the extension editing window; most of them just call the
callbacks for the main window */

void
after_ext_window_realize               (GtkWidget       *widget,
                                        gpointer         user_data)
{
    /* Create the Open Recent submenu */
    GtkRecentManager *manager = gtk_recent_manager_get_default();
    GtkWidget *recent_menu = gtk_recent_chooser_menu_new_for_manager(manager);
    GtkRecentFilter *filter = gtk_recent_filter_new();
    gtk_recent_filter_add_application(filter, "GNOME Inform 7");
    gtk_recent_chooser_set_filter(GTK_RECENT_CHOOSER(recent_menu), filter);
    g_signal_connect(recent_menu, "item-activated",
      G_CALLBACK(on_open_recent_activate), NULL);
    gtk_menu_item_set_submenu(
      GTK_MENU_ITEM(lookup_widget(widget, "xopen_recent")),
      recent_menu);
    
    /* Attach the spelling checker to the source view and ensure the correct
    state of the menu */
    if(config_file_get_bool("Settings", "SpellCheck"))
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lookup_widget(widget,
          "xautocheck_spelling")), TRUE);
    else
        gtk_widget_set_sensitive(lookup_widget(widget,"xcheck_spelling"),FALSE);
}


void
on_xclose_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    struct extension *ext = get_ext(GTK_WIDGET(menuitem));

    /* If this was the last window open, ask if we really want to quit */
    if(verify_save_ext(GTK_WIDGET(menuitem))) {
        if(get_num_app_windows() == 1) {
            GtkWidget *dialog = gtk_message_dialog_new(
              GTK_WINDOW(ext->window), GTK_DIALOG_DESTROY_WITH_PARENT,
              GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "Quit GNOME Inform 7?");
            gint result = gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            if(result != GTK_RESPONSE_YES)
                return;
        }
        delete_ext(ext);

        if(get_num_app_windows() == 0)
            gtk_main_quit();
    }
}


void
on_xsave_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    struct extension *ext = get_ext(GTK_WIDGET(menuitem));

    if(ext->filename == NULL)
        on_xsave_as_activate(menuitem, user_data);
    else
        save_extension(GTK_WIDGET(menuitem));
}


void
on_xsave_as_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    struct extension *ext = get_ext(GTK_WIDGET(menuitem));
        
    /* Create a file chooser for saving the extension */
    GtkWidget *dialog = gtk_file_chooser_dialog_new ("Save File",
      GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(menuitem))),
      GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER(dialog),
      TRUE);

    if (ext->filename == NULL) {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
          "Untitled document");
    } else
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),     
          ext->filename);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *path = gtk_file_chooser_get_filename(
          GTK_FILE_CHOOSER(dialog));
        set_ext_filename(ext, path);
        save_extension(GTK_WIDGET(menuitem));
        g_free(path);
    }

    gtk_widget_destroy (dialog);
}


void
on_xrevert_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    struct extension *ext = get_ext(GTK_WIDGET(menuitem));
    
    if(ext->filename == NULL)
        return; /* No saved version to revert to */        
    if(!gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(ext->buffer)))
        return; /* Text has not changed since last save */
    
    /* Ask if the user is sure */
    GtkWidget *revert_dialog = gtk_message_dialog_new_with_markup(
      GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(menuitem))),
      GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
      "Are you sure you want to revert to the last saved version?");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(revert_dialog),
      "All unsaved changes will be lost.");
    gtk_dialog_add_buttons(GTK_DIALOG(revert_dialog),
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_REVERT_TO_SAVED, GTK_RESPONSE_OK,
      NULL);
    gint result = gtk_dialog_run(GTK_DIALOG(revert_dialog));
    gtk_widget_destroy(revert_dialog);
    if(result != GTK_RESPONSE_OK)
        return; /* Only go on if the user clicked revert */

    /* Store the filename, close the window and reopen it */
    gchar *filename = g_strdup(ext->filename);
    delete_ext(ext);
    ext = open_extension(filename);
    g_free(filename);
    gtk_widget_show(ext->window);
}


void
on_xundo_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    struct extension *ext = get_ext(GTK_WIDGET(menuitem));

    if(gtk_source_buffer_can_undo(ext->buffer))
        gtk_source_buffer_undo(ext->buffer);
}


void
on_xredo_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    struct extension *ext = get_ext(GTK_WIDGET(menuitem));

    if(gtk_source_buffer_can_redo(ext->buffer))
        gtk_source_buffer_redo(ext->buffer);
}


void
on_xcut_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    g_signal_emit_by_name(
      G_OBJECT(lookup_widget(GTK_WIDGET(menuitem), "ext_code")),
      "cut-clipboard", NULL);
}


void
on_xcopy_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    g_signal_emit_by_name(
      G_OBJECT(lookup_widget(GTK_WIDGET(menuitem), "ext_code")),
      "copy-clipboard", NULL);
}


void
on_xpaste_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    g_signal_emit_by_name(
      G_OBJECT(lookup_widget(GTK_WIDGET(menuitem), "ext_code")),
      "paste-clipboard", NULL);
}


void
on_xselect_all_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    g_signal_emit_by_name(
      G_OBJECT(lookup_widget(GTK_WIDGET(menuitem), "ext_code")),
      "select-all", TRUE, NULL);
}


void
on_xfind_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gpointer ext = (gpointer)get_ext(GTK_WIDGET(menuitem));
    GtkWidget *dialog = create_find_dialog();
    /* Do the same thing as in the main window, but with different callbacks
    that expect an extension struct instead of a story */
    g_signal_connect((gpointer)lookup_widget(dialog, "find_next"),
      "clicked", G_CALLBACK(on_xfind_next_clicked), ext);
    g_signal_connect((gpointer)lookup_widget(dialog, "find_previous"),
      "clicked", G_CALLBACK(on_xfind_previous_clicked), ext);
    g_signal_connect((gpointer)lookup_widget(dialog, "find_replace_find"),
      "clicked", G_CALLBACK(on_xfind_replace_find_clicked), ext);
    g_signal_connect((gpointer)lookup_widget(dialog, "find_replace_all"),
      "clicked", G_CALLBACK(on_xfind_replace_all_clicked), ext);
    gtk_widget_show(dialog);
}


void
on_xautocheck_spelling_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gboolean check = gtk_check_menu_item_get_active(
      GTK_CHECK_MENU_ITEM(menuitem));
    gtk_widget_set_sensitive(
      lookup_widget(GTK_WIDGET(menuitem), "xcheck_spelling"), check);
    config_file_set_bool("Settings", "SpellCheck", check);
    /* Set the default for new windows to whatever the user chose last */
    
    if(check) {
        GError *err = NULL;
        if(!gtkspell_new_attach(
          GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(menuitem), "ext_code")),
          NULL, &err))
            error_dialog(
              GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(menuitem))),
              err, "Error initializing spell checking: ");
    } else
        gtkspell_detach(gtkspell_get_from_text_view(
          GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(menuitem), "ext_code"))));
}


void
on_xcheck_spelling_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkSpell *spellchecker = gtkspell_get_from_text_view(
      GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(menuitem), "ext_code")));
    if(spellchecker)
        gtkspell_recheck_all(spellchecker);
}


/* Create the GtkSourceView that displays the code */
GtkWidget*
ext_code_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *source = gtk_source_view_new();
    gtk_widget_set_name(source, widget_name);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(source), GTK_WRAP_WORD);
    update_font(source);
    update_tabs(GTK_SOURCE_VIEW(source));
    return source;
}


gboolean
on_ext_window_delete_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    if(verify_save_ext(widget)) {
        struct extension *ext = get_ext(GTK_WIDGET(widget));

        if(get_num_app_windows() == 1) {
            GtkWidget *dialog = gtk_message_dialog_new(NULL, 0,
              GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "Quit GNOME Inform 7?");
            gint result = gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            if(result != GTK_RESPONSE_YES)
                return TRUE;
        }

        delete_ext(ext);

        if(get_num_app_windows() == 0) {
            gtk_main_quit();
        }
        return FALSE;
    }
    return TRUE;
}


/* This function is called when the window is about to be destroyed and we can't
do anything about it. It asks whether we want to save, but doesn't offer the
option to cancel. */
void
on_ext_window_destroy                  (GtkObject       *object,
                                        gpointer         user_data)
{
    struct extension *ext = get_ext(GTK_WIDGET(object));
    if(ext == NULL)
        return;
    
    if(gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(ext->buffer))) {
        gchar *filename = g_path_get_basename(ext->filename);
        GtkWidget *save_changes_dialog = gtk_message_dialog_new_with_markup(
          GTK_WINDOW(ext->window),
          GTK_DIALOG_DESTROY_WITH_PARENT,
          GTK_MESSAGE_WARNING,
          GTK_BUTTONS_NONE,
          "<b><big>Save changes to '%s' before closing?</big></b>",
          filename);
        gtk_message_dialog_format_secondary_text(
          GTK_MESSAGE_DIALOG(save_changes_dialog),
          "If you don't save, your changes will be lost.");
        gtk_dialog_add_buttons(GTK_DIALOG(save_changes_dialog),
          "Close _without saving", GTK_RESPONSE_REJECT,
          GTK_STOCK_SAVE, GTK_RESPONSE_OK,
          NULL);
        gint result = gtk_dialog_run(GTK_DIALOG(save_changes_dialog));
        gtk_widget_destroy(save_changes_dialog);
        if(result == GTK_RESPONSE_OK)
            on_save_activate(GTK_MENU_ITEM(lookup_widget(GTK_WIDGET(object),
            "save")), NULL);
        g_free(filename);
    }
    delete_ext(ext);
}

/* Scroll to the requested line number. */
void jump_to_line_ext(GtkWidget *widget, gint line) {
    struct extension *ext = get_ext(widget);
    GtkTextIter cursor, line_end;
        
    gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(ext->buffer), &cursor,
      line - 1); /* line is counted from 0 */
    line_end = cursor;
    gtk_text_iter_forward_to_line_end(&line_end);
    gtk_text_buffer_select_range(GTK_TEXT_BUFFER(ext->buffer), &cursor,
      &line_end);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(lookup_widget(ext->window,
      "ext_code")),
    gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(ext->buffer)),
      0.25, FALSE, 0.0, 0.0);
}
