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

#include "general_header_files/enum__columns.h"
#include "general_header_files/enum__find_entry_row_buttons.h"
#include "general_header_files/enum__ts_elements.h"
#include "general_header_files/enum__view_and_options_menu_items.h"
#include "find.h"

void show_or_hide_find_grid (void);
void find_buttons_management (gchar *find_in_check_button_clicked);
static gboolean add_occurrence_to_list (GtkTreeModel G_GNUC_UNUSED *local_model, 
					GtkTreePath *local_path, GtkTreeIter *local_iter);
static inline void clear_list_of_rows_with_found_occurrences (void);
void create_list_of_rows_with_found_occurrences (void);
gboolean check_for_match (const gchar *search_term_str, GtkTreeIter *local_iter, guint8 column_number);
static gboolean ensure_visibility_of_find (GtkTreeModel G_GNUC_UNUSED *local_model, GtkTreePath *local_path, 
					   GtkTreeIter *local_iter, const gchar *search_term_str);
void run_search (void);
void jump_to_previous_or_next_occurrence (gpointer direction_pointer);

/* 

   Shows or hides (and resets the settings of) the find grid.

*/

void show_or_hide_find_grid (void)
{
  if (gtk_widget_get_visible (find_grid)) {
    gtk_entry_set_text (GTK_ENTRY (find_entry), "");
    gtk_widget_override_background_color (find_entry, GTK_STATE_NORMAL, NULL);
    gtk_widget_set_sensitive (find_button_entry_row[BACK], FALSE);
    gtk_widget_set_sensitive (find_button_entry_row[FORWARD], FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_in_all_columns), TRUE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_match_case), FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_regular_expression), TRUE);
    gtk_widget_hide (find_grid);
    clear_list_of_rows_with_found_occurrences ();
    gtk_widget_queue_draw (GTK_WIDGET (treeview)); // Force redrawing of treeview.
  }
  else {
    gtk_widget_show (find_grid);
    gtk_widget_grab_focus (find_entry);
  }
}

/* 

   (De)activates all other buttons if "All columns" is (un)selected. 
   Search results are updated for any change of the chosen columns and criteria ("match case" and "regular expression").

*/

void find_buttons_management (gchar *find_in_check_button_clicked)
{
  if (find_in_check_button_clicked) { // TRUE if any find_in_columns or find_in_all columns check buttons clicked.
    GdkRGBA bg_color_find_in_all_columns;
    gboolean marking_active;
    
    gtk_style_context_get_background_color (gtk_widget_get_style_context (find_in_all_columns), 
					    GTK_STATE_NORMAL, &bg_color_find_in_all_columns);
    
    marking_active = gdk_rgba_equal (&bg_color_find_in_all_columns, &((GdkRGBA) { 0.92, 0.73, 0.73, 1.0 } ));
    
    if (marking_active || *find_in_check_button_clicked /* TRUE if find_in_all_columns check button clicked. */) {
      gboolean find_all_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_in_all_columns));
      for (guint8 columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
	if (*find_in_check_button_clicked) { // TRUE if find_in_all_columns check button clicked.
	  g_signal_handler_block (find_in_columns[columns_cnt], handler_id_find_in_columns[columns_cnt]);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_in_columns[columns_cnt]), find_all_active);
	  g_signal_handler_unblock (find_in_columns[columns_cnt], handler_id_find_in_columns[columns_cnt]);
	  gtk_widget_set_sensitive (find_in_columns[columns_cnt], !find_all_active);
	}
	if (marking_active)
	  gtk_widget_override_background_color (find_in_columns[columns_cnt], GTK_STATE_NORMAL, NULL);
      }
      if (marking_active) {
	gtk_widget_override_background_color (find_in_all_columns, GTK_STATE_NORMAL, NULL);
	// (Note: run_search would not continue with an empty entry, but why starting it anyway?) */
	if (*search_term_str)
	  run_search (); // Update results.
      }
    }
  }

  if (*search_term_str) {
    create_list_of_rows_with_found_occurrences ();
    row_selected (); // Reset status of forward and back buttons.
  }

  gtk_widget_queue_draw (GTK_WIDGET (treeview)); // Force redrawing of treeview (for highlighting of search results).
}

/* 

   Adds a row that contains a column matching the search term to a list.

*/

static gboolean add_occurrence_to_list (GtkTreeModel G_GNUC_UNUSED *local_model, 
					GtkTreePath                *local_path, 
					GtkTreeIter                *local_iter)
{
  for (guint8 columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_in_columns[columns_cnt])) && 
	check_for_match (search_term_str, local_iter, columns_cnt)) {
      // (Note: Row references are not used here, since the list is recreated everytime the treestore is changed.)
      rows_with_found_occurrences = g_list_prepend (rows_with_found_occurrences, gtk_tree_path_copy (local_path));
      break;
    }
  }
    
  return FALSE;
}

/* 

   Clears the list of rows so it can be rebuild later.

*/

static inline void clear_list_of_rows_with_found_occurrences (void) {
  g_list_free_full (rows_with_found_occurrences, (GDestroyNotify) gtk_tree_path_free);
  rows_with_found_occurrences = NULL;
}

/* 

   Creates a list of all rows that contain at least one cell with the search term.

*/

void create_list_of_rows_with_found_occurrences (void)
{
  clear_list_of_rows_with_found_occurrences ();
  gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) add_occurrence_to_list, NULL);
  rows_with_found_occurrences = g_list_reverse (rows_with_found_occurrences);
}

/* 

   Checks for each column if it contains the search term.

*/

gboolean check_for_match (const gchar *search_term_str, 
                          GtkTreeIter *local_iter, 
                          guint8       column_number)
{
  gboolean match = FALSE; // Default
  gchar *current_column;
  
  gtk_tree_model_get (model, local_iter, column_number + TREEVIEW_COLUMN_OFFSET, &current_column, -1);
  if (current_column) {
    gchar *search_term_str_escaped = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_regular_expression))) ? 
      NULL : g_regex_escape_string (search_term_str, -1);

    if (g_regex_match_simple ((search_term_str_escaped) ? search_term_str_escaped : search_term_str, current_column, 
			      (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_match_case))) ? 
			      0 : G_REGEX_CASELESS, G_REGEX_MATCH_NOTEMPTY)) {
      match = TRUE;
    }

    // Cleanup
    g_free (search_term_str_escaped);
  }

  // Cleanup
  g_free (current_column);

  return match;
}

/* 

   If the search term is contained inside...
   ...a row whose parent is not expanded expand the latter.
   ...a column which is hidden show this column.

*/

static gboolean ensure_visibility_of_find (GtkTreeModel G_GNUC_UNUSED *local_model, 
					   GtkTreePath                *local_path, 
					   GtkTreeIter                *local_iter, 
					   const gchar                *search_term_str)
{
  for (guint8 columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_in_columns[columns_cnt])) && 
	check_for_match (search_term_str, local_iter, columns_cnt)) {
      if (gtk_tree_path_get_depth (local_path) > 1 &&
	  !gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), local_path)) {
	gtk_tree_view_expand_to_path (GTK_TREE_VIEW (treeview), local_path);
	gtk_tree_view_collapse_row (GTK_TREE_VIEW (treeview), local_path);
      }
      if (!gtk_tree_view_column_get_visible (columns[columns_cnt])) {
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[(columns_cnt == COL_MENU_ID) ? 
										 SHOW_MENU_ID_COL : 
										 SHOW_EXECUTE_COL]), TRUE);
      }
    }
  }

  return FALSE;
}

/* 

   Runs a search on the entered search term.

*/

void run_search (void)
{
  GdkRGBA missing_fields_bg_color = { 0.92, 0.73, 0.73, 1.0 };
  gboolean no_find_in_columns_buttons_clicked = TRUE; // Default

  guint8 columns_cnt;

  search_term_str = gtk_entry_get_text (GTK_ENTRY (find_entry));

  gtk_widget_override_background_color (find_entry, GTK_STATE_NORMAL, 
					(*search_term_str) ? NULL : &missing_fields_bg_color);
  
  for (columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_in_columns[columns_cnt]))) {
      no_find_in_columns_buttons_clicked = FALSE;
      break;
    }
  }

  if (!(*search_term_str) || no_find_in_columns_buttons_clicked) {
    if (no_find_in_columns_buttons_clicked) {
      for (columns_cnt = 0; columns_cnt < COL_ELEMENT_VISIBILITY; columns_cnt++)
	gtk_widget_override_background_color (find_in_columns[columns_cnt], GTK_STATE_NORMAL, &missing_fields_bg_color);
      gtk_widget_override_background_color (find_in_all_columns, GTK_STATE_NORMAL, &missing_fields_bg_color);
    }
    clear_list_of_rows_with_found_occurrences ();
  }
  else {
    create_list_of_rows_with_found_occurrences ();

    if (rows_with_found_occurrences) {
      GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

      gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) ensure_visibility_of_find, (gpointer) search_term_str);

      gtk_tree_selection_unselect_all (selection);
      gtk_tree_selection_select_path (selection, rows_with_found_occurrences->data);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview), rows_with_found_occurrences->data, NULL, FALSE, 0, 0);
    }
  }

  row_selected (); // Reset status of forward and back buttons.

  gtk_widget_queue_draw (GTK_WIDGET (treeview)); // Force redrawing of treeview (for highlighting of search results).
}

/* 

   Enables the possibility to move between the found occurrences.

*/

void jump_to_previous_or_next_occurrence (gpointer direction_pointer)
{
  gboolean forward = GPOINTER_TO_INT (direction_pointer);

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  GtkTreePath *path = gtk_tree_model_get_path (model, &iter), *path_of_occurrence;
  GtkTreeIter iter_of_occurrence;

  GList *rows_with_found_occurrences_loop;

  for (rows_with_found_occurrences_loop = (forward) ? 
	 rows_with_found_occurrences : g_list_last (rows_with_found_occurrences); 
       rows_with_found_occurrences_loop; 
       rows_with_found_occurrences_loop = (forward) ? rows_with_found_occurrences_loop->next : 
	 rows_with_found_occurrences_loop->prev)
    if ((forward) ? (gtk_tree_path_compare (path, rows_with_found_occurrences_loop->data) < 0) : 
	(gtk_tree_path_compare (path, rows_with_found_occurrences_loop->data) > 0))
      break;
  
  path_of_occurrence = rows_with_found_occurrences_loop->data;
  gtk_tree_model_get_iter (model, &iter_of_occurrence, path_of_occurrence);

  ensure_visibility_of_find (NULL, path_of_occurrence, &iter_of_occurrence, search_term_str);

  gtk_tree_selection_unselect_all (selection);
  gtk_tree_selection_select_path (selection, path_of_occurrence);
  gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview), path_of_occurrence, NULL, FALSE, 0, 0);
  
  // Cleanup
  gtk_tree_path_free (path);
}
