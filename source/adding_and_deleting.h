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

#ifndef __adding_and_deleting_h
#define __adding_and_deleting_h

#define streq(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

extern const guint8 NUMBER_OF_ACTIONS;
extern gchar *actions[];

extern const guint8 NUMBER_OF_EXECUTE_OPTS;
extern gchar *execute_options[];
extern gchar *execute_displayed_txt[];

extern const guint8 NUMBER_OF_STARTUPNOTIFY_OPTS;
extern gchar *startupnotify_options[];
extern gchar *startupnotify_displayed_txt[];

extern GtkTreeStore *treestore;
extern GtkTreeModel *model;
extern GtkWidget *treeview;
extern GtkTreeIter iter;

extern GtkWidget *mb_edit, *mb_search, *mb_options;

extern GtkToolItem *tb[];

extern GtkWidget *button_grid;
extern GtkWidget *bt_add_action_option_label;

extern GtkListStore *action_option_combo_box_liststore;
extern GtkTreeModel *action_option_combo_box_model;
extern GtkWidget *action_option_grid;
extern GtkWidget *action_option, *action_option_done;
extern GtkWidget *options_grid, *suboptions_grid;
extern GtkWidget *options_command_label, *options_prompt_label, *options_check_button_label;
extern GtkWidget *options_command, *options_prompt, *options_check_button;
extern GtkWidget *suboptions_labels[], *suboptions_fields[];

extern GtkWidget *find_grid;

extern gchar *txt_fields[];
extern GSList *menu_ids;

extern gboolean autosort_options;

gchar *search_term_str;

extern gint handler_id_row_selected, handler_id_action_option_combo_box, handler_id_show_startupnotify_options;

extern void activate_change_done (void);
extern void check_for_existing_options (GtkTreeIter *parent, guint8 number_of_opts, 
					gchar **options_array, gboolean *opts_exist);
extern gchar *check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *path);
extern GtkWidget *create_dialog (GtkWidget **dialog, gchar *dialog_title, gchar *stock_id, gchar *button_txt_1, 
				 gchar *button_txt_2, gchar *button_txt_3, gchar *label_txt, gboolean show_immediately);
extern void row_selected (void);
extern void show_or_hide_find_grid (void);
extern void sort_execute_or_startupnotify_options_after_insertion (gchar *execute_or_startupnotify,
								   GtkTreeSelection *selection,
								   GtkTreeIter *parent, gchar *option);
G_GNUC_NULL_TERMINATED extern gboolean streq_any (const gchar *string, ...);

#endif
