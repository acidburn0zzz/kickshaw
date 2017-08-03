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
#include <string.h>

#include "general_header_files/enum__action_option_combo.h"
#include "general_header_files/enum__startupnotify_options.h"
#include "general_header_files/enum__toolbar_buttons.h"
#include "general_header_files/enum__txt_fields.h"
#include "general_header_files/enum__ts_elements.h"
#include "adding_and_deleting.h"

void add_new (gchar *new_element_type);
void option_list_with_headlines (GtkCellLayout G_GNUC_UNUSED *cell_layout, 
				 GtkCellRenderer *action_option_combo_box_renderer, 
				 GtkTreeModel *action_option_combo_box_model, 
				 GtkTreeIter *action_option_combo_box_iter, 
				 gpointer G_GNUC_UNUSED data);
static void generate_action_option_combo_box_content_for_Execute_and_startupnotify_opts (gchar *type);
void generate_action_option_combo_box (gchar *preset_choice);
void show_action_options (void);
void show_startupnotify_options (void);
void single_field_entry (void);
void hide_action_option (void);
static void clear_entries (void);
static void expand_row_from_iter (GtkTreeIter *local_iter);
void action_option_insert (gchar *origin);
static gboolean check_for_menus (GtkTreeModel *filter_model, GtkTreePath G_GNUC_UNUSED *filter_path, 
				 GtkTreeIter *filter_iter);
void remove_menu_id (gchar *menu_id);
void remove_all_children (void);
void remove_rows (gchar *origin);

/* 

   Quick addition of a new menu element without the necessity to enter any value.

*/

void add_new (gchar *new_element_type)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  GtkTreeIter new_iter;
  GtkTreePath *path = NULL; // Default

  // Defaults
  gboolean treestore_is_empty_or_no_selection = FALSE;
  gboolean path_is_on_toplevel = TRUE;
  gboolean insertion = TRUE;

  G_GNUC_EXTENSION gchar *new_ts_fields[] = { [0 ... NUMBER_OF_TS_ELEMENTS] = NULL }; // Defaults

  guint8 ts_fields_cnt;

  /* If the selection currently points to a menu a decision has to be made to 
     either put the new element inside that menu or next to it on the same level. */
  if (streq (txt_fields[TYPE_TXT], "menu")) {
    GtkWidget *dialog;
    gchar *label_txt;

    gint result;

    enum { INSIDE_MENU = 2, CANCEL };

    label_txt = g_strdup_printf ("Insert new %s on the same level or into the currently selected menu?", 
				 new_element_type);
 
    create_dialog (&dialog, "Same level or inside menu?", GTK_STOCK_DIALOG_QUESTION, 
		   "Same level", "Inside menu", "Cancel", label_txt, TRUE);

    // Cleanup
    g_free (label_txt);
 
    result = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    switch (result) {
    case INSIDE_MENU:
      insertion = FALSE;
      break;
    case CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
      return;
    }
  }

  if (gtk_tree_selection_count_selected_rows (selection)) { // == 1
    path = gtk_tree_model_get_path (model, &iter);
    if (gtk_tree_path_get_depth (path) > 1)
      path_is_on_toplevel = FALSE;
  }
  else
    treestore_is_empty_or_no_selection = TRUE;

  // Adding options inside an action or an option block.
  if (streq (txt_fields[TYPE_TXT], "action") || 
      (streq (txt_fields[TYPE_TXT], "option block") && !streq_any (new_element_type, "prompt", "command", NULL))) {
    insertion = FALSE;
  }

  								  
  // --- Assigning values. ---


  // Predefinitions
  if (!streq (new_element_type, "separator")) {
    new_ts_fields[TS_MENU_ELEMENT] = g_strconcat (streq_any (new_element_type, "menu", "pipe menu", "item", NULL) ? 
						  "New " : "", new_element_type, NULL);
  }
  new_ts_fields[TS_TYPE] = new_element_type;

  if (streq_any (new_element_type, "menu", "pipe menu", NULL)) {
    gchar *new_menu_str;
    guint menu_id_index = 1;

    // Menu IDs have to be unique, so the list of menu IDs has to be checked for existing values.
    while (g_slist_find_custom (menu_ids, new_menu_str = g_strdup_printf ("New menu %i", menu_id_index++), 
				(GCompareFunc) strcmp)) {
      g_free (new_menu_str);
    }
    new_ts_fields[TS_MENU_ID] = new_menu_str;
    menu_ids = g_slist_prepend (menu_ids, new_menu_str);
  }
  else if (!streq_any (new_element_type, "item", "separator", NULL)) { // Option
    new_ts_fields[TS_TYPE] = "option"; // Overwrite predefinition.
    if (streq (new_element_type, "enabled"))
      new_ts_fields[TS_VALUE] = "no";
    else if (streq_any (txt_fields[MENU_ELEMENT_TXT], "Exit", "SessionLogout", NULL))
      new_ts_fields[TS_VALUE] = "yes";
    else
      new_ts_fields[TS_VALUE] = "";
  }


  // --- Add the new row. ---

  
  // Prevents that the default check for change of selection(s) gets in the way.
  g_signal_handler_block (selection, handler_id_row_selected);

  gtk_tree_selection_unselect_all (selection); // The old selection will be replaced by the one of the new row.

  if (treestore_is_empty_or_no_selection || !insertion) {
    gtk_tree_store_append (treestore, &new_iter, (treestore_is_empty_or_no_selection) ? NULL : &iter);

    if (!treestore_is_empty_or_no_selection)
      gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), path, FALSE);
  }
  else {
    GtkTreeIter parent;

    if (!path_is_on_toplevel)
      gtk_tree_model_iter_parent (model, &parent, &iter);
    gtk_tree_store_insert_after (treestore, &new_iter, (path_is_on_toplevel) ? NULL : &parent, &iter);
  }

  // Set element visibility of menus, pipe menus, items and separators.
  if (streq_any (new_element_type, "menu", "pipe menu", "item", "separator", NULL)) {
    new_ts_fields[TS_ELEMENT_VISIBILITY] = "visible"; // Default
    if (path) {
      GtkTreePath *new_path = gtk_tree_model_get_path (model, &new_iter);
      gchar *invisible_ancestor_txt = check_if_invisible_ancestor_exists (model, new_path);

      if (invisible_ancestor_txt) {
	new_ts_fields[TS_ELEMENT_VISIBILITY] = (g_str_has_suffix (invisible_ancestor_txt, "invisible menu")) ?
	  "invisible dsct. of invisible menu" : "invisible dsct. of invisible unintegrated menu";	   
      }

      // Cleanup
      gtk_tree_path_free (new_path);
      g_free (invisible_ancestor_txt);
    }
  }

  for (ts_fields_cnt = 0; ts_fields_cnt < NUMBER_OF_TS_ELEMENTS; ts_fields_cnt++)
    gtk_tree_store_set (treestore, &new_iter, ts_fields_cnt, new_ts_fields[ts_fields_cnt], -1);

  // Cleanup
  g_free (new_ts_fields[TS_MENU_ELEMENT]);
  gtk_tree_path_free (path);

  gtk_tree_selection_select_iter (selection, &new_iter);

  /* In case that autosorting is activated:
     If the type of the new row is...
     ...a prompt or command option of Execute, ... OR:
     ...an option of startupnotify, ...
     ...sort all options of the latter and move the selection to the new option. */
  if (autosort_options && streq (new_ts_fields[TS_TYPE], "option")) {
    GtkTreeIter parent;

    gtk_tree_model_iter_parent (model, &parent, &new_iter);
    if (gtk_tree_model_iter_n_children (model, &parent) > 1)
      sort_execute_or_startupnotify_options_after_insertion ((streq_any (new_element_type, "prompt", "command", NULL)) 
							     ? "Execute" : "startupnotify", 
							     selection, &parent, new_element_type);
  }

  g_signal_handler_unblock (selection, handler_id_row_selected);

  row_selected ();
  activate_change_done ();
}

/* 

   Sets up a cell layout that includes headlines that are highlighted in colour.

*/

void option_list_with_headlines (GtkCellLayout G_GNUC_UNUSED *cell_layout, 
				 GtkCellRenderer             *action_option_combo_box_renderer, 
				 GtkTreeModel                *action_option_combo_box_model, 
				 GtkTreeIter                 *action_option_combo_box_iter, 
				 gpointer G_GNUC_UNUSED       data)
{
  gchar *action_option_combo_item;
  GdkRGBA normal_fg_color, normal_bg_color;
  
  gtk_style_context_get_color (gtk_widget_get_style_context (action_option), GTK_STATE_NORMAL, &normal_fg_color);
  gtk_style_context_get_background_color (gtk_widget_get_style_context (action_option), GTK_STATE_NORMAL, 
					  &normal_bg_color);
  
  gtk_tree_model_get (action_option_combo_box_model, action_option_combo_box_iter, 
		      ACTION_OPTION_COMBO_ITEM, &action_option_combo_item, 
		      -1);

  g_object_set (action_option_combo_box_renderer, "foreground-rgba", 
		(g_regex_match_simple ("Add|Choose an", action_option_combo_item, G_REGEX_ANCHORED, 0)) ? 
		&((GdkRGBA) { 1.0, 1.0, 1.0, 1.0 }) : &normal_fg_color, "foreground-set", TRUE, "background-rgba", 
		(g_str_has_prefix (action_option_combo_item, "Choose an ")) ? &((GdkRGBA) { 0.31, 0.31, 0.79, 1.0 }) : 
		((g_str_has_prefix (action_option_combo_item, "Add ")) ? 
		 &((GdkRGBA) { 0.0, 0.0, 0.0, 1.0 }) : &normal_bg_color), "background-set", TRUE, "sensitive", 
		!g_regex_match_simple ("Add|Choose an", action_option_combo_item, G_REGEX_ANCHORED, 0), NULL);

  // Cleanup
  g_free (action_option_combo_item);
}

/* 

   Appends possible options for Execute or startupnotify to combobox.

*/

static void generate_action_option_combo_box_content_for_Execute_and_startupnotify_opts (gchar *type)
{
  gboolean execute = streq (type, "Execute");
  GtkTreeIter parent, action_option_combo_box_new_iter;
  const guint8 number_of_opts = (execute) ? NUMBER_OF_EXECUTE_OPTS : NUMBER_OF_STARTUPNOTIFY_OPTS;
  gboolean *opts_exist = g_malloc0 (number_of_opts * sizeof (gboolean)); // Initialise all elements to FALSE.

  guint8 opts_cnt;

  if (streq (txt_fields[MENU_ELEMENT_TXT], type)) // "Execute" or "startupnotify"
    parent = iter;
  else
    gtk_tree_model_iter_parent (model, &parent, &iter);

  check_for_existing_options (&parent, number_of_opts, (execute) ? execute_options : startupnotify_options, opts_exist);

  for (opts_cnt = 0; opts_cnt < number_of_opts; opts_cnt++) {
    if (!opts_exist[opts_cnt]) {
      gtk_list_store_insert_with_values (action_option_combo_box_liststore, &action_option_combo_box_new_iter, -1, 
					 ACTION_OPTION_COMBO_ITEM, (execute) ? 
					 execute_displayed_txt[opts_cnt] : startupnotify_displayed_txt[opts_cnt],
					 -1);
    }
  }

  // Cleanup
  g_free (opts_exist);
}

/* 

   Shows the action combo box, to which a dynamically generated list of options has been added.

*/

void generate_action_option_combo_box (gchar *preset_choice)
{
  GtkTreeIter action_option_combo_box_iter;
  gchar *action_option_combo_item_first, *action_option_combo_item_last;

  const gchar *bt_add_action_option_label_txt;

  gtk_widget_set_sensitive (action_option, TRUE);

 
  // --- Build combo list. ---


  // Prevents that the signal connected to the combo box gets in the way.
  g_signal_handler_block (action_option, handler_id_action_option_combo_box);

  // Headline
  gtk_list_store_clear (action_option_combo_box_liststore);
  // Generate "Choose..." headline according to the text of bt_add_action_option_label.
  bt_add_action_option_label_txt = gtk_label_get_text (GTK_LABEL (bt_add_action_option_label));
  gtk_list_store_insert_with_values (action_option_combo_box_liststore, &action_option_combo_box_iter, -1, 
				     ACTION_OPTION_COMBO_ITEM,
				     (streq (bt_add_action_option_label_txt, "Action")) ? "Choose an action" : 
				     (streq (bt_add_action_option_label_txt, "Option") 
				      ? "Choose an option" : "Choose an action/option"),
				     -1);

  // Actions
  if (streq_any (txt_fields[TYPE_TXT], "item", "action", NULL)) { 
    for (guint8 actions_cnt = 0; actions_cnt < NUMBER_OF_ACTIONS; actions_cnt++) {
      gtk_list_store_insert_with_values (action_option_combo_box_liststore, &action_option_combo_box_iter, -1, 
					 ACTION_OPTION_COMBO_ITEM, actions[actions_cnt], 
					 -1);
    }
  }

  // Execute options: prompt, command and startupnotify
  if ((streq (txt_fields[TYPE_TXT], "action") && streq (txt_fields[MENU_ELEMENT_TXT], "Execute")) ||
      (streq_any (txt_fields[TYPE_TXT], "option", "option block", NULL) && 
       streq_any (txt_fields[MENU_ELEMENT_TXT], "prompt", "command", "startupnotify", NULL)))
    generate_action_option_combo_box_content_for_Execute_and_startupnotify_opts ("Execute");

  // Startupnotify options: enabled, name, wmclass, icon
  if (streq_any (txt_fields[TYPE_TXT], "option", "option block", NULL) && 
      streq_any (txt_fields[MENU_ELEMENT_TXT], "startupnotify", "enabled", "name", "wmclass", "icon", NULL))
    generate_action_option_combo_box_content_for_Execute_and_startupnotify_opts ("startupnotify");

  // Exit and SessionLogout option: prompt, Restart option: command
  if (streq (txt_fields[TYPE_TXT], "action") && 
      streq_any (txt_fields[MENU_ELEMENT_TXT], "Exit", "Restart", "SessionLogout", NULL) && 
      !gtk_tree_model_iter_has_child (model, &iter)) {
    gtk_list_store_insert_with_values (action_option_combo_box_liststore, &action_option_combo_box_iter, -1, 
				       ACTION_OPTION_COMBO_ITEM, 
				       (streq (txt_fields[MENU_ELEMENT_TXT], "Restart")) ? "Command" : "Prompt", 
				       -1);
  }

  /* Check if there are two kinds of addable actions/options: the first one next to the current selection, 
     the second one inside the current selection. If so, add headlines to the list to separate them from each other. */
  gtk_tree_model_iter_nth_child (action_option_combo_box_model, &action_option_combo_box_iter, NULL, 1);
  gtk_tree_model_get (action_option_combo_box_model, &action_option_combo_box_iter,
		      ACTION_OPTION_COMBO_ITEM, &action_option_combo_item_first, 
		      -1);
  gtk_tree_model_iter_nth_child (action_option_combo_box_model, &action_option_combo_box_iter, NULL, 
				 gtk_tree_model_iter_n_children (action_option_combo_box_model, NULL) - 1);
  gtk_tree_model_get (action_option_combo_box_model, &action_option_combo_box_iter, 
		      ACTION_OPTION_COMBO_ITEM, &action_option_combo_item_last, 
		      -1);

  if (streq (action_option_combo_item_first, "Execute") && 
      streq_any (action_option_combo_item_last, "Prompt", "Command", "Startupnotify", NULL)) {
    gchar *add_inside_txt;

    gtk_list_store_insert_with_values (action_option_combo_box_liststore, &action_option_combo_box_iter, 1, 
				       ACTION_OPTION_COMBO_ITEM, "Add additional action", 
				       -1);

    /* "Add additional action" + all possible actions (defined by number_of_actions) + 
       "Add inside currently selected xxx action" -> NUM_ACTIONS + 2 */
    add_inside_txt = g_strdup_printf ("Add inside currently selected %s action", txt_fields[MENU_ELEMENT_TXT]);
    gtk_list_store_insert_with_values (action_option_combo_box_liststore, &action_option_combo_box_iter, 
				       NUMBER_OF_ACTIONS + 2, ACTION_OPTION_COMBO_ITEM, add_inside_txt, 
				       -1);

    // Cleanup
    g_free (add_inside_txt);
  }
  else if (streq_any (action_option_combo_item_first, "Prompt", "Command", NULL) &&
	   streq_any (action_option_combo_item_last, "Enabled", "Name", "WM_CLASS", "Icon", NULL)) {
    gchar *action_option_combo_item;

    gtk_list_store_insert_with_values (action_option_combo_box_liststore, &action_option_combo_box_iter, 1, 
				       ACTION_OPTION_COMBO_ITEM, "Add next to startupnotify", 
				       -1);
    /* 
       The value of position NUMBER_OF_EXECUTE_OPTS determines if an offset 
       is necessary for the insertion of the second headline.
       Add action/option:                                                            0
       Add next to startupnotify:                                                    1
       Prompt/Command:                                                               2
       Command OR startupnotify option if there is only one possible Execute option: 3 (= NUMBER_OF_EXECUTE_OPTS)
    */
    gtk_tree_model_iter_nth_child (action_option_combo_box_model, &action_option_combo_box_iter, 
				   NULL, NUMBER_OF_EXECUTE_OPTS);
    gtk_tree_model_get (action_option_combo_box_model, &action_option_combo_box_iter, 
			ACTION_OPTION_COMBO_ITEM, &action_option_combo_item, 
			-1);

    gtk_list_store_insert_with_values (action_option_combo_box_liststore, &action_option_combo_box_iter,
    				       NUMBER_OF_EXECUTE_OPTS + ((streq (action_option_combo_item, "Command")) ? 1 : 0), 
    				       ACTION_OPTION_COMBO_ITEM, "Add inside startupnotify",
				       -1);

    // Cleanup
    g_free (action_option_combo_item);
  }

  // Cleanup
  g_free (action_option_combo_item_first);
  g_free (action_option_combo_item_last);

  // Show the action combo box and deactivate or hide all events and widgets that could interfere.
  gtk_widget_set_sensitive (mb_edit, FALSE);
  gtk_widget_set_sensitive (mb_search, FALSE);
  gtk_widget_set_sensitive (mb_options, FALSE);
  for (guint8 tb_cnt = TB_MOVE_UP; tb_cnt <= TB_FIND; tb_cnt++)
    gtk_widget_set_sensitive ((GtkWidget *) tb[tb_cnt], FALSE);
  gtk_widget_hide (button_grid);
  gtk_widget_hide (find_grid);

  gtk_widget_show (action_option_grid);
  gtk_combo_box_set_active (GTK_COMBO_BOX (action_option), 0);
  gtk_widget_hide (action_option_done);
  gtk_widget_hide (options_grid);

  g_signal_handler_unblock (action_option, handler_id_action_option_combo_box);

  gtk_widget_queue_draw (GTK_WIDGET (treeview)); // Force redrawing of treeview (if a search was active).

  /* If this function was called by the context menu or a "Startupnotify" button, 
     preselect the appropriate combo box item. */
  if (preset_choice) {
    gtk_combo_box_set_active_id (GTK_COMBO_BOX (action_option), preset_choice);
    // If there is only one choice, there is nothing to choose from the combo box.
    if (gtk_tree_model_iter_n_children (action_option_combo_box_model, NULL) == 2)
      gtk_widget_set_sensitive (action_option, FALSE);
  }
}

/* 

   Shows the fields that may be changed according to the chosen action.

*/

void show_action_options (void)
{
  const gchar *combo_choice = gtk_combo_box_get_active_id (GTK_COMBO_BOX (action_option));
  
  guint8 snotify_opts_cnt;

  clear_entries ();

  gtk_widget_show (action_option_done);

  // Defaults
  if (!streq (combo_choice, "Reconfigure"))
    gtk_widget_show (options_grid);
  else
    gtk_widget_hide (options_grid);
  gtk_widget_hide (options_command_label);
  gtk_widget_hide (options_command);
  gtk_widget_hide (options_prompt_label);
  gtk_widget_hide (options_prompt);
  gtk_widget_hide (options_check_button_label);
  gtk_widget_hide (options_check_button);
  gtk_widget_hide (suboptions_grid);
  for (snotify_opts_cnt = 0; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
    gtk_widget_show (suboptions_labels[snotify_opts_cnt]);
    gtk_widget_show (suboptions_fields[snotify_opts_cnt]);
  }
  gtk_widget_grab_focus (options_command);

  // "Execute" action
  if (streq (combo_choice, "Execute")) {
    gtk_widget_show (options_command_label);
    gtk_widget_show (options_command);
    gtk_label_set_text (GTK_LABEL (options_prompt_label), "Prompt (optional): ");
    gtk_widget_show (options_prompt_label);
    gtk_widget_show (options_prompt);
    gtk_label_set_text (GTK_LABEL (options_check_button_label), "Startupnotify ");
    gtk_widget_show (options_check_button_label);
    if (!g_signal_handler_is_connected (options_check_button, handler_id_show_startupnotify_options)) {
      handler_id_show_startupnotify_options = g_signal_connect (options_check_button, "toggled", 
								G_CALLBACK (show_startupnotify_options), NULL);
    }
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options_check_button), FALSE);
    gtk_widget_show (options_check_button);
  }

  // "Restart" action or just "command" option (both for "Restart" and "Execute" actions)
  if (streq_any (combo_choice, "Restart", "Command", NULL)) {
    gtk_widget_show (options_command_label);
    gtk_widget_show (options_command);
  }

  // "Exit" or "SessionLogout" action or just "prompt" option (for "Exit", "SessionLogout" and "Execute" actions)
  if (streq_any (combo_choice, "Exit", "Prompt", "SessionLogout", NULL)) {
    if (streq_any (combo_choice, "Exit", "SessionLogout", NULL) || 
	(streq (combo_choice, "Prompt") && streq_any (txt_fields[MENU_ELEMENT_TXT], "Exit", "SessionLogout", NULL))) {
      gtk_label_set_text (GTK_LABEL (options_check_button_label), " Prompt: ");
      gtk_widget_show (options_check_button_label);
      if (g_signal_handler_is_connected (options_check_button, handler_id_show_startupnotify_options))
	g_signal_handler_disconnect (options_check_button, handler_id_show_startupnotify_options);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options_check_button), TRUE);
      gtk_widget_show (options_check_button);
      gtk_widget_grab_focus (options_check_button);
    }
    else {
      gtk_label_set_text (GTK_LABEL (options_prompt_label), " Prompt: ");
      gtk_widget_show (options_prompt_label);
      gtk_widget_show (options_prompt);
      gtk_widget_grab_focus (options_prompt);
    }
  }

  // "startupnotify" option and its suboptions
  if (streq_any (combo_choice, "Startupnotify", "Enabled", "Name", "WM_CLASS", "Icon", NULL)) {
    gtk_widget_show (suboptions_grid);
    // Enabled, Name, WM_CLASS & Icon
    if (streq (combo_choice, "Startupnotify")) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (suboptions_fields[ENABLED]), TRUE);
      gtk_widget_grab_focus (suboptions_fields[ENABLED]);
    }
    else {
      for (snotify_opts_cnt = 0; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
	if (!streq (combo_choice, startupnotify_displayed_txt[snotify_opts_cnt])) {
	  gtk_widget_hide (suboptions_labels[snotify_opts_cnt]);
	  gtk_widget_hide (suboptions_fields[snotify_opts_cnt]);
	}
	else
	  gtk_widget_grab_focus (suboptions_fields[snotify_opts_cnt]);
      }
    }
  }
}

/* 

   Shows the options for startupnotify after the corresponding check box has been clicked.

*/

void show_startupnotify_options (void)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options_check_button)))
    gtk_widget_show (suboptions_grid);
  else
    gtk_widget_hide (suboptions_grid);
}

/* 

   If only a single entry field is shown, "Enter" is enough, clicking on "Done" is not necessary.

*/

void single_field_entry (void)
{
  if (streq_any (gtk_combo_box_get_active_id (GTK_COMBO_BOX (action_option)), 
		 "Command", "Icon", "Name", "Prompt", "Restart", "WM_CLASS", NULL))
    action_option_insert ("by combo box");
}

/* 

   Resets everything after the cancel button has been clicked or an insertion has been done.

*/

void hide_action_option (void)
{
  clear_entries ();
  gtk_widget_set_sensitive (mb_options, TRUE);
  gtk_widget_hide (action_option_grid);
  gtk_widget_show (button_grid);
  if (*search_term_str)
    gtk_widget_show (find_grid);
  row_selected (); // Reset visibility of menu bar and toolbar items.
}

/*

  Clears all entries for actions/options.

*/

static void clear_entries (void)
{
  guint8 snotify_opts_cnt;

  gtk_entry_set_text (GTK_ENTRY (options_command), "");
  gtk_entry_set_text (GTK_ENTRY (options_prompt), "");
  gtk_widget_override_background_color (options_command, GTK_STATE_NORMAL, NULL);
  gtk_widget_override_background_color (options_prompt, GTK_STATE_NORMAL, NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options_check_button), FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (suboptions_fields[ENABLED]), FALSE);
  for (snotify_opts_cnt = NAME; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++)
    gtk_entry_set_text (GTK_ENTRY (suboptions_fields[snotify_opts_cnt]), "");
}

/* 

   Auxiliary function for expanding a row from an iterator.

*/

static void expand_row_from_iter (GtkTreeIter *local_iter)
{
  GtkTreePath *path = gtk_tree_model_get_path (model, local_iter);

  gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), path, FALSE);

  // Cleanup
  gtk_tree_path_free (path);
}

/* 

   Inserts an action and/or its option(s) with the entered values.

*/

void action_option_insert (gchar *origin)
{
  const gchar *choice = (streq (origin, "by combo box")) ? gtk_combo_box_get_active_id (GTK_COMBO_BOX (action_option)) : 
    "Reconfigure";
  gboolean options_check_button_state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options_check_button));
  gboolean option1_check_button_state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (suboptions_fields[0]));
  const gchar *options_command_entry = gtk_entry_get_text (GTK_ENTRY (options_command));
  const gchar *options_prompt_entry = gtk_entry_get_text (GTK_ENTRY (options_prompt));

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  GtkTreePath *path;
  GtkTreeIter new_iter, new_iter2, parent, execute_parent, execute_iter;

  gint insertion_position = -1; // Default

  // Defaults
  gboolean action = FALSE;
  gboolean execute_done = FALSE;
  gboolean startupnotify_done = FALSE;
  gboolean option_of_execute = FALSE;

  enum { PROMPT, COMMAND, STARTUPNOTIFY, NUMBER_OF_EXECUTE_OPTS };
  // Defaults
  G_GNUC_EXTENSION gboolean used_Execute_opts[] = { [0 ... NUMBER_OF_EXECUTE_OPTS] = FALSE };
  guint8 snotify_start = NAME;

  guint8 execute_opts_cnt, snotify_opts_cnt;

  // If only one entry or a mandatory field was displayed, check if it has been filled out.
  if (gtk_widget_get_visible (action_option_grid)) {
    GdkRGBA entry_bg_color = { 0.92, 0.73, 0.73, 1.0 };

    if (streq_any (choice, "Execute", "Command", NULL) && !(*options_command_entry)) {
      gtk_widget_override_background_color (options_command, GTK_STATE_NORMAL, &entry_bg_color);
      return;
    }
    else if (streq (choice, "Prompt") && // "Execute" action or option of it currently selected.
	     !streq_any (txt_fields[MENU_ELEMENT_TXT], "Exit", "SessionLogout", NULL) && !(*options_prompt_entry)) {
      gtk_widget_override_background_color (options_prompt, GTK_STATE_NORMAL, &entry_bg_color);
      return;
    }
    else {
      for (snotify_opts_cnt = NAME; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
	if (streq (choice, startupnotify_displayed_txt[snotify_opts_cnt]) && 
	    !(*gtk_entry_get_text (GTK_ENTRY (suboptions_fields[snotify_opts_cnt])))) {
	  gtk_widget_override_background_color (suboptions_fields[snotify_opts_cnt], GTK_STATE_NORMAL, &entry_bg_color);
	  return;
	}
      }
    }
  }

  // Check if an insertion has to be done.
  if (!(streq (txt_fields[TYPE_TXT], "item") || 
	(streq (txt_fields[TYPE_TXT], "action") && 
	 streq_any (choice, "Prompt", "Command", "Startupnotify", NULL)) || 
	(streq (txt_fields[TYPE_TXT], "option block") && 
	 !streq_any (choice, "Prompt", "Command", NULL)))) {
    path = gtk_tree_model_get_path (model, &iter);
    gint last_path_index = gtk_tree_path_get_indices (path)[gtk_tree_path_get_depth (path) - 1];

    gtk_tree_model_iter_parent (model, &parent, &iter);
    if (gtk_tree_model_iter_n_children (model, &parent) > 1 && 
	last_path_index < gtk_tree_model_iter_n_children (model, &parent) - 1) {
      insertion_position = last_path_index + 1;
    }

    // Cleanup
    gtk_tree_path_free (path);
  }


  // --- Create the new row(s). ---


  // Prevents that the default check for change of selection(s) gets in the way.
  g_signal_handler_block (selection, handler_id_row_selected);
  gtk_tree_selection_unselect_all (selection); // The old selection will be replaced by the one of the new row.

  // Execute
  if (streq (choice, "Execute")) {
    if (streq (txt_fields[TYPE_TXT], "action")) {
      gtk_tree_model_iter_parent (model, &parent, &iter);
      iter = parent; // Move up one level and start from there.
    }

    gtk_tree_store_insert_with_values (treestore, &new_iter, &iter, insertion_position,
				       TS_MENU_ELEMENT, "Execute",
				       TS_TYPE, "action",
				       -1);

    execute_parent = iter;
    execute_iter = new_iter;
    iter = new_iter; // New base level

    action = TRUE;
    execute_done = TRUE;
    insertion_position = -1; // Reset for the options, since they should be always appended.
  }

  // Prompt - only if it is an Execute option and not empty.
  if ((execute_done || 
       (streq (choice, "Prompt") && !streq_any (txt_fields[MENU_ELEMENT_TXT], "Exit", "SessionLogout", NULL))) && 
      *options_prompt_entry)
    used_Execute_opts[PROMPT] = TRUE;

  // Command - only if it is an Execute option.
  if ((execute_done || 
       (streq (choice, "Command") && !streq (txt_fields[MENU_ELEMENT_TXT], "Restart"))))
    used_Execute_opts[COMMAND] = TRUE;

  // Startupnotify
  if ((execute_done && options_check_button_state) || streq (choice, "Startupnotify"))
    used_Execute_opts[STARTUPNOTIFY] = TRUE;

  // Insert Execute option, if used.
  for (execute_opts_cnt = 0; execute_opts_cnt < NUMBER_OF_EXECUTE_OPTS; execute_opts_cnt++) {
    if (used_Execute_opts[execute_opts_cnt]) {
      if (streq_any (txt_fields[TYPE_TXT], "option", "option block", NULL)) {
	gtk_tree_model_iter_parent (model, &parent, &iter);
	iter = parent; // Move up one level and start from there.
      } 
      gtk_tree_store_insert_with_values (treestore, &new_iter, &iter, insertion_position,
					 TS_MENU_ELEMENT, (execute_opts_cnt == PROMPT) ? "prompt" : 
					 ((execute_opts_cnt == COMMAND) ? "command" : "startupnotify"), 
					 TS_TYPE, (execute_opts_cnt != STARTUPNOTIFY) ? "option" : "option block",
					 TS_VALUE, (execute_opts_cnt == PROMPT) ? options_prompt_entry : 
					 ((execute_opts_cnt == COMMAND) ? options_command_entry : NULL), 
					 -1);

      if (!execute_done) {
	option_of_execute = TRUE;
	expand_row_from_iter (&iter);
	gtk_tree_selection_select_iter (selection, &new_iter);
	if (execute_opts_cnt == STARTUPNOTIFY)
	  startupnotify_done = TRUE;
      }
    }
  }

  // Setting parent iterator for startupnotify options.
  if (streq_any (txt_fields[TYPE_TXT], "option", "option block", NULL) && 
      !streq_any (txt_fields[MENU_ELEMENT_TXT], "prompt", "command", NULL)) {
    if (streq (txt_fields[MENU_ELEMENT_TXT], "startupnotify"))
      new_iter = iter;
    else
      gtk_tree_model_iter_parent (model, &new_iter, &iter);
  }

  // Enabled
  if ((execute_done && options_check_button_state) || streq_any (choice, "Startupnotify", "Enabled", NULL))
    snotify_start = ENABLED;

  // Insert startupnotify option, if used.
  for (snotify_opts_cnt = snotify_start; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
    if (snotify_opts_cnt == ENABLED || streq (choice, startupnotify_displayed_txt[snotify_opts_cnt]) ||
  	*gtk_entry_get_text (GTK_ENTRY (suboptions_fields[snotify_opts_cnt]))) { // Part of Execute option.
      
      gtk_tree_store_insert_with_values (treestore, &new_iter2, &new_iter, insertion_position,
					 TS_MENU_ELEMENT, startupnotify_options[snotify_opts_cnt],
					 TS_TYPE, "option",
					 TS_VALUE, (snotify_opts_cnt == ENABLED) ? 
					 ((option1_check_button_state) ? "yes" : "no") : 
					 gtk_entry_get_text (GTK_ENTRY (suboptions_fields[snotify_opts_cnt])),
					 -1);

      expand_row_from_iter (&new_iter);
      if (!startupnotify_done)
	gtk_tree_selection_select_iter (selection, &new_iter2);
    }
  }

  // Action other than Execute
  if (streq_any (choice, "Exit", "Restart", "Reconfigure", "SessionLogout", NULL)) {
    if (streq (txt_fields[TYPE_TXT], "action")) {
      gtk_tree_model_iter_parent (model, &parent, &iter);
      iter = parent; // Move up one level and start from there.
    }

    gtk_tree_store_insert_with_values (treestore, &new_iter, &iter, insertion_position,
				       TS_MENU_ELEMENT, choice,
				       TS_TYPE, "action",
				       -1);
 
    if (!streq (choice, "Reconfigure")) {
      gtk_tree_store_insert_with_values (treestore, &new_iter2, &new_iter, -1, 
					 TS_MENU_ELEMENT, (streq (choice, "Restart")) ? "command" : "prompt", 
					 TS_TYPE, "option",
					 TS_VALUE, (streq (choice, "Restart") ? options_command_entry : 
						    (options_check_button_state) ? "yes" : "no"),
					 -1);
    }

    action = TRUE;
  }

  // Exit & SessionLogout option (prompt) or Restart option (command)
  if ((streq (choice, "Prompt") && streq_any (txt_fields[MENU_ELEMENT_TXT], "Exit", "SessionLogout", NULL)) || 
      (streq (choice, "Command") && streq (txt_fields [MENU_ELEMENT_TXT], "Restart"))) {
    gtk_tree_store_insert_with_values (treestore, &new_iter, &iter, -1, 
				       TS_MENU_ELEMENT, (streq (choice, "Prompt")) ? "prompt" : "command", 
				       TS_TYPE, "option",
				       TS_VALUE, ((streq (choice, "Command")) ? options_command_entry : 
						  (options_check_button_state) ? "yes" : "no"),
				       -1);

    expand_row_from_iter (&iter);
    gtk_tree_selection_select_iter (selection, &new_iter);
  }

  /* Show all children of the new action. If the parent row had not been expanded, expand it is well, 
     but leave all preceding nodes (if any) of the new action collapsed. */
  if (action) {
    if (execute_done)
      iter = execute_parent;
    GtkTreePath *parent_path = gtk_tree_model_get_path (model, &iter);
    path = gtk_tree_model_get_path (model, (execute_done) ? &execute_iter : &new_iter);

    if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), parent_path))
      gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), parent_path, FALSE);
    gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), path, TRUE);
    gtk_tree_selection_select_path (selection, path);

    // Cleanup
    gtk_tree_path_free (parent_path);
    gtk_tree_path_free (path);
  }

  if (autosort_options) {
    // If the new row is an option of Execute, sort all options of the latter and move the selection to the new option.
    if (option_of_execute) {
      gchar *option = g_ascii_strdown (choice, -1);
      if (gtk_tree_model_iter_n_children (model, &iter) > 1)
	sort_execute_or_startupnotify_options_after_insertion ("Execute", selection, &iter, option);

      // Cleanup
      g_free (option);
    }

    // The same for startupnotify.
    if (streq_any (choice, "Enabled", "Name", "WM_CLASS", "Icon", NULL) && 
	gtk_tree_model_iter_n_children (model, &new_iter) > 1) { // new_iter is always "option block" = startupnotify.
      for (snotify_opts_cnt = ENABLED; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
	if (streq (choice, startupnotify_displayed_txt[snotify_opts_cnt])) {
	  sort_execute_or_startupnotify_options_after_insertion ("Startupnotify", selection, 
								 &new_iter, startupnotify_options[snotify_opts_cnt]);
	  break;
	}
      }
    }
  }

  hide_action_option ();

  g_signal_handler_unblock (selection, handler_id_row_selected);
  row_selected ();
  activate_change_done ();
}

/* 

   Deletes menu IDs of menus or pipe menus that are descendants of the currently processed row.

*/

static gboolean check_for_menus (GtkTreeModel              *filter_model, 
				 GtkTreePath G_GNUC_UNUSED *filter_path, 
				 GtkTreeIter               *filter_iter)
{
  gchar *type_txt_filter, *menu_id_txt_filter;

  gtk_tree_model_get (filter_model, filter_iter, 
		      TS_TYPE, &type_txt_filter, 
		      TS_MENU_ID, &menu_id_txt_filter, 
		      -1);

  if (streq_any (type_txt_filter, "menu", "pipe menu", NULL))
    remove_menu_id (menu_id_txt_filter);

  // Cleanup
  g_free (type_txt_filter);
  g_free (menu_id_txt_filter);

  return FALSE;
}

/* 

   Removes a menu ID from the menu IDs list.

*/

void remove_menu_id (gchar *menu_id)
{
  GSList *to_be_removed_element = g_slist_find_custom (menu_ids, menu_id, (GCompareFunc) strcmp);

  g_free (to_be_removed_element->data);
  menu_ids = g_slist_remove (menu_ids, to_be_removed_element->data);
}

/* 

   Removes all children of a node.
   Children are removed from bottom to top so the paths of the other children and nodes are kept.
   After the removal of the children, the function checks for the existence of each parent
   before it reselects them, because if subnodes of the selected nodes had also been selected,
   they can't be reselected again since they got removed.

*/

void remove_all_children (void)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  /* The list of rows is reverted, since only with a bottom to top direction the paths of selected 
     rows will stay the same after one of them has been deleted and selected subrows of rows that 
     have also been selected will be removed properly. */
  GList *selected_rows = g_list_reverse (gtk_tree_selection_get_selected_rows (selection, &model));

  GList *g_list_loop;
  GtkTreeIter iter_loop;
  GtkTreePath *path_loop;

  // Prevents that the default check for change of selection(s) gets in the way.
  g_signal_handler_block (selection, handler_id_row_selected); 

  for (g_list_loop = selected_rows; g_list_loop; g_list_loop = g_list_loop->next) {
    path_loop = g_list_loop->data;
    // (Note: Rows are not selected if they are not visible.)
    gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), path_loop, FALSE);
    gtk_tree_selection_unselect_path (selection, path_loop);
    gtk_tree_path_down (path_loop);
    gtk_tree_model_get_iter (model, &iter_loop, path_loop);
    do {
      gtk_tree_selection_select_iter (selection, &iter_loop);
    } while (gtk_tree_model_iter_next (model, &iter_loop));
    gtk_tree_path_up (path_loop);
  }

  remove_rows ("remove all children");

  g_signal_handler_unblock (selection, handler_id_row_selected);

  for (g_list_loop = selected_rows; g_list_loop; g_list_loop = g_list_loop->next) {
    path_loop = g_list_loop->data;
    // Might be a former subnode that was selected and got removed.
    if (gtk_tree_model_get_iter (model, &iter_loop, path_loop))
      gtk_tree_selection_select_path (selection, path_loop);
  }
  
  // Cleanup
  g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}

/* 

   Removes all currently selected rows.

*/

void remove_rows (gchar *origin)
{
  GtkTreeIter iter_remove;
  GtkTreeModel *filter_model;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  // The list of rows is reverted, since only with a bottom to top direction the paths will stay the same.
  GList *selected_rows = g_list_reverse (gtk_tree_selection_get_selected_rows (selection, &model));

  gchar *type_txt_to_be_deleted;
  gchar *menu_id_txt_to_be_deleted;

  GList *g_list_loop;
  GtkTreePath *path_loop;

  // Prevents that the default check for change of selection(s) gets in the way.
  g_signal_handler_block (selection, handler_id_row_selected); 

  for (g_list_loop = selected_rows; g_list_loop; g_list_loop = g_list_loop->next) {
    path_loop = g_list_loop->data;
    gtk_tree_model_get_iter (model, &iter_remove, path_loop);

    gtk_tree_model_get (model, &iter_remove, 
			TS_TYPE, &type_txt_to_be_deleted, 
			TS_MENU_ID, &menu_id_txt_to_be_deleted, 
			-1);

    if (!streq (origin, "dnd") && streq_any (type_txt_to_be_deleted, "menu", "pipe menu", NULL)) {
      // Keep menu IDs in the GSList equal to the menu IDs of the treestore.
      remove_menu_id (menu_id_txt_to_be_deleted);

      /* If the row to be deleted is a menu and one or more children are not selected 
	 and of menu or pipe menu type, then their menu IDs have to be deleted, too. */
      if (streq (type_txt_to_be_deleted, "menu") && gtk_tree_model_iter_has_child (model, &iter_remove)) {
	filter_model = gtk_tree_model_filter_new (model, path_loop);
	gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) check_for_menus, NULL);
      }
    }

    gtk_tree_store_remove (GTK_TREE_STORE (model), &iter_remove);

    // Cleanup
    g_free (type_txt_to_be_deleted);
    g_free (menu_id_txt_to_be_deleted);
  }

  // If all rows have been deleted and the search functionality had been activated before, deactivate the latter.
  if (!gtk_tree_model_get_iter_first (model, &iter_remove) && gtk_widget_get_visible (find_grid))
    show_or_hide_find_grid ();

  g_signal_handler_unblock (selection, handler_id_row_selected);

  // Cleanup
  g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);

  if (!streq (origin, "dnd")) {
    if (!streq (origin, "load menu"))
      row_selected ();
    activate_change_done ();
  }
}
