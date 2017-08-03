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

#ifndef __kickshaw_h
#define __kickshaw_h

extern void action_option_insert (gchar *origin);
extern void add_new (gchar *new_element_type);
extern void boolean_toogled (void);
extern void hide_action_option (void);
extern void change_row (void);
extern gboolean check_for_external_file_and_settings_changes (gpointer G_GNUC_UNUSED identifier);
extern gboolean check_for_match (const gchar *search_term_str, GtkTreeIter *local_iter, guint8 column_number);
extern void create_context_menu (GdkEventButton *event);
extern void create_list_of_rows_with_found_occurrences (void);
extern void cell_edited (GtkCellRendererText G_GNUC_UNUSED *renderer, gchar *path, 
			 gchar *new_text, gpointer column_number_pointer);
extern gboolean drag_motion_handler (GtkWidget G_GNUC_UNUSED *widget, GdkDragContext *drag_context, 
				     gint x, gint y, guint time);
extern void drag_data_received_handler (GtkWidget G_GNUC_UNUSED *widget, GdkDragContext G_GNUC_UNUSED *context, 
					gint x, gint y);
extern void find_buttons_management (gchar *find_in_check_button_clicked);
extern void free_elements_of_static_string_array (gchar **string_array, gint8 number_of_fields, gboolean set_to_NULL);
extern guint get_font_size (void);
extern void get_tree_row_data (gchar *new_filename);
extern void icon_choosing_by_button_or_context_menu (void);
extern void key_pressed (GtkWidget G_GNUC_UNUSED *widget, GdkEventKey *event);
extern void jump_to_previous_or_next_occurrence (gpointer direction_pointer);
extern void move_selection (gpointer direction_pointer);
extern void open_menu (void);
extern void option_list_with_headlines (GtkCellLayout G_GNUC_UNUSED *cell_layout, 
					GtkCellRenderer *action_option_combo_box_renderer, 
					GtkTreeModel *action_option_combo_box_model, 
					GtkTreeIter *action_option_combo_box_iter, 
					gpointer G_GNUC_UNUSED data);
extern void remove_all_children (void);
extern void remove_icons_from_menus_or_items (void);
extern void remove_rows (gchar *origin);
extern void row_selected (void);
extern void run_search (void);
extern void save_menu (void);
extern void save_menu_as (gchar *save_as_filename);
extern void set_status_of_expand_and_collapse_buttons_and_menu_items (void);
extern void show_action_options (void);
extern void show_or_hide_find_grid (void);
extern void show_startupnotify_options (void);
extern void single_field_entry (void);
extern gboolean sort_loop_after_sorting_activation (GtkTreeModel *local_model, GtkTreePath G_GNUC_UNUSED *local_path,
						    GtkTreeIter *local_iter);
extern void stop_timer (void);
G_GNUC_NULL_TERMINATED extern gboolean streq_any (const gchar *string, ...);
extern void unref_icon (GdkPixbuf **icon, gboolean set_to_NULL);
extern void visualise_menus_items_and_separators (gpointer recursively_pointer);

#endif
