/*
   Kickshaw - A Menu Editor for Openbox

   Copyright (c) 2010-2013        Marcus Schaetzle

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along 
   with Kickshaw. If not, see http://www.gnu.org/licenses/.
*/

#include <gtk/gtk.h>
#include <stdlib.h>

#include "auxiliary.h"

void create_file_dialog (GtkWidget **dialog, gchar *dialog_title);
gchar *extract_substring_via_regex (gchar *string, gchar *regex_str);
void free_elements_of_static_string_array (gchar **string_array, gint8 number_of_fields, gboolean set_to_NULL);
guint get_font_size (void);
void set_filename_and_window_title (gchar *new_filename);
void show_msg_in_statusbar (gchar *message);
gboolean streq_any (const gchar *string, ...);
void unref_icon (GdkPixbuf **icon, gboolean set_to_NULL);

/* 

   Creates a file dialog for opening a new and saving an existing menu.

*/

void create_file_dialog (GtkWidget **dialog, gchar *dialog_title) {
  gchar *menu_folder;
  GtkFileFilter *file_filter;

  *dialog = gtk_file_chooser_dialog_new (dialog_title,
					 GTK_WINDOW (window),
					 (g_str_has_prefix (dialog_title, "Open")) ? 
					 GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					 (g_str_has_prefix (dialog_title, "Open")) ? GTK_STOCK_OPEN : GTK_STOCK_SAVE, 
					 GTK_RESPONSE_ACCEPT,
					 NULL);

  menu_folder = g_strconcat (getenv ("HOME"), "/.config/openbox", NULL);
  if (g_file_test (menu_folder, G_FILE_TEST_EXISTS))
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (*dialog), menu_folder);
  file_filter = gtk_file_filter_new ();

  // Cleanup
  g_free (menu_folder);

  gtk_file_filter_set_name (file_filter, "Openbox Menu Files");
  gtk_file_filter_add_pattern (file_filter, "*.xml");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (*dialog), file_filter);
}

/* 

   Extracts a substring from a string by using a regular expression.

*/

gchar *extract_substring_via_regex (gchar *string, 
				    gchar *regex_str)
{
  GRegex *regex = g_regex_new (regex_str, 0, 0, NULL);
  GMatchInfo *match_info; // If match_info is not NULL then it is created even if g_regex_match returns FALSE.
  gchar *match;

  g_regex_match (regex, string, 0, &match_info);
  match = g_strdup (g_match_info_fetch (match_info, 0));

  // Cleanup
  g_regex_unref (regex);
  g_match_info_free (match_info);

  return match;
}

/* 

   Frees dyn. alloc. memory of all strings of a static string array.

*/

void free_elements_of_static_string_array (gchar    **string_array, 
                                           gint8      number_of_fields, 
                                           gboolean   set_to_NULL)
{
  while (--number_of_fields + 1) {
    g_free (string_array[number_of_fields]);

    if (set_to_NULL)
      string_array[number_of_fields] = NULL;
  }
}

/* 

   Retrieves the current font size so the size of the icon images can be adjusted to it.

*/

guint get_font_size (void)
{
  gchar *font_str;
  gchar *font_substr;
  guint font_size;

  g_object_get (gtk_settings_get_default (), "gtk-font-name", &font_str, NULL);
  font_size = atoi (font_substr = extract_substring_via_regex (font_str, "[0-9]+$"));

  // Cleanup
  g_free (font_str);
  g_free (font_substr);

  return font_size;
}

/* 

   Replaces the filename and window title.

*/

void set_filename_and_window_title (gchar *new_filename)
{
  gchar *basename;
  gchar *window_title;

  free_and_reassign (filename, new_filename);
  basename = g_path_get_basename (filename);
  window_title = g_strconcat ("Kickshaw - ", basename, NULL);
  gtk_window_set_title (GTK_WINDOW (window), window_title);

  // Cleanup
  g_free (basename);
  g_free (window_title);
}

/* 

   Shows a message in the statusbar at the botton for information.

*/

void show_msg_in_statusbar (gchar *message)
{
  message = g_strdup_printf (" %s", message);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), message), 
		      message);

  // Cleanup
  g_free (message);
}

/* 

   Short replacement for the check if a string equals to one of several values
   (strcmp (x,y) == 0 || strcmp (x,z) == 0).

*/

gboolean streq_any (const gchar *string, ...)
{
  va_list arguments;
  gchar *check;

  va_start (arguments, string);
  while ((check = va_arg (arguments, gchar *))) { // Parenthesis avoid gcc warning.
    if (streq (string, check)) {
      va_end (arguments); // Mandatory for safety and implementation/platform neutrality.
      return TRUE;
    }
  }
  va_end (arguments); // Mandatory for safety and implementation/platform neutrality.
  return FALSE;
}

/* 

   Unrefs icon pixbuf and sets it to NULL, if desired so.

*/

void unref_icon (GdkPixbuf **icon, 
                 gboolean    set_to_NULL)
{
  if (*icon) {
    g_object_unref (*icon);

    if (set_to_NULL)
      *icon = NULL;
  }
}

