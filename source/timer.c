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

#include "general_header_files/enum__entry_fields.h"
#include "general_header_files/enum__invalid_icon_imgs.h"
#include "general_header_files/enum__invalid_icon_imgs_status.h"
#include "general_header_files/enum__ts_elements.h"
#include "timer.h"

void stop_timer (void);
gboolean check_for_external_file_and_settings_changes (gpointer G_GNUC_UNUSED identifier);

/* 

   Stops the timer and erases the list of icon occourrences.

*/

void stop_timer (void)
{
  g_source_remove_by_user_data ("timer");
  g_slist_free_full (rows_with_icons, (GDestroyNotify) gtk_tree_row_reference_free);
  rows_with_icons = NULL;
}

/* 

   Checks if...
   ...a valid icon file path has become invalid, if so, changes the icon to a broken one.
   ...an invalid icon file path has become valid, if so, replaces the broken icon with the icon image,
      if it is a proper image file.
   ...the font size has been changed, if so, changes the size of icons according to the new font size.

*/

// The identifier is not used by the function, but to switch the timer on and off.
gboolean check_for_external_file_and_settings_changes (gpointer G_GNUC_UNUSED identifier) {
  gboolean recreate_icon_images;
  guint font_size_updated;

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gint number_of_selected_rows = gtk_tree_selection_count_selected_rows (selection);

  GtkTreeIter iter_loop;
  GtkTreePath *path_loop;

  GdkPixbuf *icon_pixbuf_loop;
  guint icon_img_status_uint_loop;
  gchar *icon_modified_txt_loop;
  gchar *icon_path_txt_loop;

  GSList *rows_with_icons_loop;

  if ((recreate_icon_images = (font_size != (font_size_updated = get_font_size ())))) {
    font_size = font_size_updated;
    create_invalid_icon_imgs ();
  }

  for (rows_with_icons_loop = rows_with_icons; 
       rows_with_icons_loop; 
       rows_with_icons_loop = rows_with_icons_loop->next) {
    path_loop = gtk_tree_row_reference_get_path (rows_with_icons_loop->data);
    gtk_tree_model_get_iter (model, &iter_loop, path_loop);
 
    gtk_tree_model_get (model, &iter_loop,
			TS_ICON_IMG, &icon_pixbuf_loop, 
			TS_ICON_IMG_STATUS, &icon_img_status_uint_loop, 
			TS_ICON_MODIFIED, &icon_modified_txt_loop,
			TS_ICON_PATH, &icon_path_txt_loop, 
			-1);

    /* Case 1: Stored path no longer points to a valid icon image, so icon has to replaced with broken icon.
       Case 2: Invalid path to an icon image has become valid.
       Case 3: Icon image file is replaced with another one. */
    if (icon_img_status_uint_loop != INVALID_PATH && !g_file_test (icon_path_txt_loop, G_FILE_TEST_EXISTS)) {
      gtk_tree_store_set (treestore, &iter_loop, 
			  TS_ICON_IMG, invalid_icon_imgs[INVALID_PATH_ICON], 
			  TS_ICON_IMG_STATUS, INVALID_PATH, 
			  TS_ICON_MODIFIED, NULL, 
			  -1);

      if (number_of_selected_rows == 1 && gtk_tree_selection_iter_is_selected (selection, &iter_loop))
	gtk_widget_override_background_color (entry_fields[ICON_PATH_ENTRY], GTK_STATE_NORMAL, 
					      &((GdkRGBA) { 0.92, 0.73, 0.73, 1.0 } ));
    }
    else if (g_file_test (icon_path_txt_loop, G_FILE_TEST_EXISTS)) {
      gchar *time_stamp = get_modified_date_for_icon (icon_path_txt_loop);

      if (icon_img_status_uint_loop == INVALID_PATH || !streq (time_stamp, icon_modified_txt_loop)) {
	if (!set_icon (icon_path_txt_loop, &iter_loop, TRUE)) {
	  gtk_tree_store_set (treestore, &iter_loop, 
			      TS_ICON_IMG, invalid_icon_imgs[INVALID_FILE_ICON],
			      TS_ICON_IMG_STATUS, INVALID_FILE, 
			      TS_ICON_MODIFIED, g_strdup (time_stamp),
			      -1);
	}

	if (number_of_selected_rows == 1 && gtk_tree_selection_iter_is_selected (selection, &iter_loop))
	  gtk_widget_override_background_color (entry_fields[ICON_PATH_ENTRY], GTK_STATE_NORMAL, NULL);
      }

      // Cleanup
      g_free (time_stamp);
    }

    // Font size has changed, so reload the icon images to adjust them to the new font size.
    if (recreate_icon_images) {
      if (!icon_img_status_uint_loop)
	set_icon (icon_path_txt_loop, &iter_loop, TRUE);
      else {
	gtk_tree_store_set (treestore, &iter_loop, TS_ICON_IMG, 
			    invalid_icon_imgs[(icon_img_status_uint_loop == INVALID_PATH) ? INVALID_PATH_ICON : 
					      INVALID_FILE_ICON], -1);
      }

      gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview)); // In case that font size is reduced.
    }

    // Cleanup
    gtk_tree_path_free (path_loop);
    g_object_unref (icon_pixbuf_loop);
    g_free (icon_modified_txt_loop);
    g_free (icon_path_txt_loop);
  }

  return TRUE;
}
