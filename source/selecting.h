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

#ifndef __selecting_h
#define __selecting_h

#define streq(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

extern gchar *execute_options[];
extern gchar *execute_displayed_txt[];
extern const guint8 NUMBER_OF_EXECUTE_OPTS;
#define STARTUPNOTIFY 2 // 2 = index inside execute_options.

extern gchar *startupnotify_options[];
extern gchar *startupnotify_displayed_txt[];
extern const guint8 NUMBER_OF_STARTUPNOTIFY_OPTS;

extern GtkTreeModel *model;
extern GtkWidget *treeview;
extern GtkTreeIter iter;

extern GtkWidget *mb_file_menu_items[];
extern GtkWidget *mb_edit;
extern GtkWidget *mb_edit_menu_items[];
extern GtkWidget *mb_search;
extern GtkWidget *mb_expand_all_nodes, *mb_collapse_all_nodes;

extern GtkToolItem *tb[];

extern GtkWidget *add_image;
extern GtkWidget *bt_bar_label;
extern GtkWidget *bt_add[];
extern GtkWidget *bt_add_action_option_label;

extern GtkWidget *action_option_grid;

extern GtkWidget *find_button_entry_row[];
extern GList *rows_with_found_occurrences;

extern GtkWidget *entry_grid;
extern GtkWidget *entry_labels[], *entry_fields[];
extern GtkWidget *icon_chooser, *remove_icon;

extern gchar *txt_fields[];

extern gboolean change_done;
extern gboolean autosort_options;
extern gchar *filename;

extern GtkTargetEntry enable_list[];
extern GSList *source_paths;

extern gint handler_id_action_option_button_clicked;

extern void add_new (gchar *new_element_type);
extern void hide_action_option (void);
extern void check_for_existing_options (GtkTreeIter *parent, guint8 number_of_opts, 
					gchar **options_array, gboolean *opts_exist);
extern gboolean check_if_invisible_descendant_exists (GtkTreeModel *filter_model,
						      GtkTreePath G_GNUC_UNUSED *filter_path,
						      GtkTreeIter *filter_iter, 
						      gboolean *at_least_one_descendant_is_invisible);
extern void free_elements_of_static_string_array (gchar **string_array, gint8 number_of_fields, gboolean set_to_NULL);
extern void generate_action_option_combo_box (gchar *preset_choice);
extern void show_msg_in_statusbar (gchar *message);
G_GNUC_NULL_TERMINATED extern gboolean streq_any (const gchar *string, ...);

#endif
