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

#ifndef __context_menu_h
#define __context_menu_h

#define streq(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

extern GtkTreeModel *model;
extern GtkWidget *treeview;
extern GtkTreeIter iter;

extern gchar *txt_fields[];

extern const guint8 NUMBER_OF_ACTIONS;
extern gchar *actions[];
#define RECONFIGURE 2 // 2 = index inside actions.

extern const guint8 NUMBER_OF_EXECUTE_OPTS;
extern gchar *execute_options[];
#define STARTUPNOTIFY 2 // 2 = index inside execute_options.

extern const guint8 NUMBER_OF_STARTUPNOTIFY_OPTS;
extern gchar *startupnotify_options[];

extern void action_option_insert (gchar *cm_choice);
extern void add_new (gchar *new_element_type);
extern void check_for_existing_options (GtkTreeIter *parent, guint8 number_of_opts, 
					gchar **options_array, gboolean *opts_exist);
extern gboolean check_if_invisible_descendant_exists (GtkTreeModel *filter_model,
						      GtkTreePath G_GNUC_UNUSED *filter_path,
						      GtkTreeIter *filter_iter, 
						      gboolean *at_least_one_descendant_is_invisible);
extern void generate_action_option_combo_box (gchar *preset_choice);
extern void icon_choosing_by_button_or_context_menu (void);
extern void remove_all_children (void);
extern void remove_icons_from_menus_or_items (void);
extern void remove_rows (gchar *origin);
G_GNUC_NULL_TERMINATED extern gboolean streq_any (const gchar *string, ...);
extern void visualise_menus_items_and_separators (gpointer recursively_pointer);

#endif
