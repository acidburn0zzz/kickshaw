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
#include <stdbool.h>

#include "general_header_files/enum__txt_fields.h"
#include "general_header_files/enum__ts_elements.h"
#include "general_header_files/struct__expansion_status_data.h"
#include "context_menu.h"

enum { RECURSIVELY, IMMEDIATE, COLLAPSE, NUMBER_OF_EXPANSION_STATUS_CHANGES };

static void create_cm_headline (GtkWidget *context_menu, gchar *cm_text);
static gboolean check_expansion_status_of_subnodes (GtkTreeModel *filter_model, GtkTreePath *filter_path,
						    GtkTreeIter *filter_iter, 
						    struct expansion_status_data *expansion_status_of_subnodes);
static void expand_or_collapse_selected_rows (gpointer action_pointer);
static void add_startupnotify_or_execute_options_to_context_menu (GtkWidget *context_menu, gboolean startupnotify_opts, 
								  GtkTreeIter *parent, guint8 number_of_opts, 
								  gchar **options_array);
void create_context_menu (GdkEventButton *event);

/* 

   Creates a headline inside the context menu.

*/

static void create_cm_headline (GtkWidget *context_menu, 
				gchar     *cm_text) 
{
  GtkWidget *eventbox = gtk_event_box_new ();
  GtkWidget *headline_label = gtk_label_new (cm_text);
  GtkWidget *menu_item = gtk_menu_item_new ();
  GdkColor headline_color;

  gtk_misc_set_alignment (GTK_MISC (headline_label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (eventbox), headline_label);
  gdk_color_parse ("#656772", &headline_color);
  gtk_widget_modify_bg (eventbox, GTK_STATE_NORMAL, &headline_color);
  gdk_color_parse ("white", &headline_color);
  gtk_widget_modify_fg (headline_label, GTK_STATE_NORMAL, &headline_color);
  gtk_container_add (GTK_CONTAINER (menu_item), eventbox);
  gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
}

/* 

   Checks if subnodes are expanded or collapsed.

*/

static gboolean check_expansion_status_of_subnodes (GtkTreeModel                 *filter_model, 
						    GtkTreePath                  *filter_path,
						    GtkTreeIter                  *filter_iter, 
						    struct expansion_status_data *expansion_status_of_subnodes)
{
  if (gtk_tree_model_iter_has_child (filter_model, filter_iter)) {
    // The path of the model, not filter model, is needed to check whether the row is expanded.
    GtkTreePath *model_path = gtk_tree_model_filter_convert_path_to_child_path ((GtkTreeModelFilter *) filter_model, 
										filter_path);

    if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), model_path)) {
      expansion_status_of_subnodes->at_least_one_is_expanded = 1;
      if (gtk_tree_path_get_depth (filter_path) == 1)
	expansion_status_of_subnodes->at_least_one_imd_ch_is_exp = 1;
    }
    else
      expansion_status_of_subnodes->at_least_one_is_collapsed = 1;

    // Cleanup
    gtk_tree_path_free (model_path);
  }

  return (expansion_status_of_subnodes->at_least_one_is_expanded && 
	  expansion_status_of_subnodes->at_least_one_is_collapsed && 
	  expansion_status_of_subnodes->at_least_one_imd_ch_is_exp); // Stop if all bit fields are set.
}

/* 

   Expands or collapses all selected rows according to the choice done.

*/

static void expand_or_collapse_selected_rows (gpointer action_pointer)
{
  guint8 action = GPOINTER_TO_UINT (action_pointer);

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  GList *selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
  GtkTreePath *path;
  
  GList *g_list_loop;

  for (g_list_loop = selected_rows; g_list_loop; g_list_loop = g_list_loop->next) {
    path = g_list_loop->data;
    /* If the nodes are already expanded recursively and only the immediate children shall be expanded now, 
       it is the fastest way to collapse all nodes first. */
    gtk_tree_view_collapse_row (GTK_TREE_VIEW (treeview), path);
    if (action == COLLAPSE)
      gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview));
    else
      gtk_tree_view_expand_row (GTK_TREE_VIEW (treeview), path, (action == RECURSIVELY));
  }
  
  // Cleanup
  g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}

/* 

   Adds all currently unused startupnotify or Execute options to the context menu.

*/

static void add_startupnotify_or_execute_options_to_context_menu (GtkWidget    *context_menu, 
								  gboolean      startupnotify_opts, 
								  GtkTreeIter  *parent, 
								  guint8        number_of_opts, 
								  gchar       **options_array)
{
  gboolean *opts_exist = g_malloc0 (number_of_opts * sizeof (gboolean)); // Initialise all elements to FALSE.
  gchar *menu_item_txt;
  GtkWidget *menu_item;

  guint8 opts_cnt;

  check_for_existing_options (parent, number_of_opts, options_array, opts_exist);

  for (opts_cnt = 0; opts_cnt < number_of_opts; opts_cnt++) {
    if (!opts_exist[opts_cnt]) {
      menu_item_txt = g_strconcat ("Add ", options_array[opts_cnt], NULL);
      menu_item = gtk_menu_item_new_with_label (menu_item_txt);

      // Cleanup
      g_free (menu_item_txt);

      if (startupnotify_opts || opts_cnt != STARTUPNOTIFY)
	g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (add_new), options_array[opts_cnt]);
      else
	g_signal_connect_swapped (menu_item, "activate", 
				  G_CALLBACK (generate_action_option_combo_box), "Startupnotify");
      gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
    }
  }

  // Cleanup
  g_free (opts_exist);
}

/* 

   Right click on the treeview presents a context menu for adding, changing and deleting menu elements.

*/

void create_context_menu (GdkEventButton *event)
{
  GtkWidget *context_menu;
  GtkWidget *menu_item;
  gchar *menu_item_txt;

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gint number_of_selected_rows = gtk_tree_selection_count_selected_rows (selection);
  GList *selected_rows = NULL;
  GtkTreePath *path = NULL;

  GtkTreeIter iter_loop;
  GList *g_list_loop;

  gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview), event->x, event->y, &path, NULL, NULL, NULL);

  context_menu = gtk_menu_new ();
  
  if (path) {
    if (number_of_selected_rows > 1) {
      // Defaults
      gboolean selected_row_is_one_of_the_previously_selected_ones = FALSE;
      gboolean at_least_one_row_has_no_icon = FALSE;

      GdkPixbuf *icon_img_pixbuf_loop;

      /* If several rows are selected and a row that is not one of these is rightclicked, 
	 select the latter and unselect the former ones. */
      selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
      for (g_list_loop = selected_rows; g_list_loop; g_list_loop = g_list_loop->next)
	if (gtk_tree_path_compare (path, g_list_loop->data) == 0)
	  selected_row_is_one_of_the_previously_selected_ones = TRUE;
      if (!selected_row_is_one_of_the_previously_selected_ones) {
	gtk_tree_selection_unselect_all (selection);
	gtk_tree_selection_select_path (selection, path);
	// Reset
	g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
	selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
	number_of_selected_rows = 1;
      }

      // Add context menu item for deletion of icons if all selected rows contain icons.
      for (g_list_loop = selected_rows; g_list_loop; g_list_loop = g_list_loop->next) {
	gtk_tree_model_get_iter (model, &iter_loop, g_list_loop->data);
	gtk_tree_model_get (model, &iter_loop, TS_ICON_IMG, &icon_img_pixbuf_loop, -1);
	if (!icon_img_pixbuf_loop) {
	  at_least_one_row_has_no_icon = TRUE;
	  break;
	}
	// Cleanup
	g_object_unref (icon_img_pixbuf_loop);
      }
      if (!at_least_one_row_has_no_icon) {
	menu_item = gtk_menu_item_new_with_label ("Remove icons");
	g_signal_connect (menu_item, "activate", G_CALLBACK (remove_icons_from_menus_or_items), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
      }
    }
    /* Either no or one row had been selected before or 
       two or more rows had been selected before and another row not one of these has now been rightclicked. */
    if (number_of_selected_rows < 2) {
      gtk_tree_selection_unselect_all (selection);
      gtk_tree_selection_select_path (selection, path);
      selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);
      number_of_selected_rows = 1; // if there had not been a selection before.

      // Icon
      if (streq_any (txt_fields[TYPE_TXT], "menu", "pipe menu", "item", NULL)) {
	if (streq (txt_fields[TYPE_TXT], "item"))
	  create_cm_headline (context_menu, " Icon"); // Items have "Icon" and "Action" as context menu headlines.
	menu_item = gtk_menu_item_new_with_label (!(txt_fields[ICON_PATH_TXT]) ? "Add icon" : "Change icon");
	g_signal_connect (menu_item, "activate", G_CALLBACK (icon_choosing_by_button_or_context_menu), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
	if (txt_fields[ICON_PATH_TXT]) {
	  menu_item = gtk_menu_item_new_with_label ("Remove icon");
	  g_signal_connect (menu_item, "activate", G_CALLBACK (remove_icons_from_menus_or_items), NULL);
	  gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
	}
	if (!streq (txt_fields[TYPE_TXT], "item")) // Items have "Icon" and "Action" as context menu headlines.
	  gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
      }

      if (gtk_tree_path_get_depth (path) > 1) {
	GtkTreeIter parent;
	
	// Startupnotify options
	if (streq_any (txt_fields[TYPE_TXT], "option", "option block", NULL) 
	    && streq_any (txt_fields[MENU_ELEMENT_TXT], "startupnotify", "enabled", "name", "wmclass", "icon", NULL)) {
	  if (!streq (txt_fields[MENU_ELEMENT_TXT], "startupnotify"))
	    gtk_tree_model_iter_parent (model, &parent, &iter);
	  else
	    parent = iter;
	  if (gtk_tree_model_iter_n_children (model, &parent) < NUMBER_OF_STARTUPNOTIFY_OPTS) {
	    GtkTreeIter grandparent;

	    gtk_tree_model_iter_parent (model, &grandparent, &parent);
	    if (streq (txt_fields[TYPE_TXT], "option block") && 
		gtk_tree_model_iter_n_children (model, &grandparent) < NUMBER_OF_EXECUTE_OPTS) {
	      create_cm_headline (context_menu, 
				  (gtk_tree_model_iter_n_children (model, &parent) == 
				   NUMBER_OF_STARTUPNOTIFY_OPTS - 1) ?
				  " Startupnotify option" : " Startupnotify options");
	    }

	    add_startupnotify_or_execute_options_to_context_menu (context_menu, TRUE, &parent, 
								  NUMBER_OF_STARTUPNOTIFY_OPTS, startupnotify_options);

	    // No "Execute option(s)" headline following.
	    if (gtk_tree_model_iter_n_children (model, &grandparent) == NUMBER_OF_EXECUTE_OPTS || 
		streq (txt_fields[TYPE_TXT], "option"))
	      gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
	  }
	}

	gchar *menu_element_txt_parent;
	gtk_tree_model_iter_parent (model, &parent, &iter);
	gtk_tree_model_get (model, &parent, TS_MENU_ELEMENT, &menu_element_txt_parent, -1);
	// Execute options
	if ((streq (txt_fields[TYPE_TXT], "action") && streq (txt_fields[MENU_ELEMENT_TXT], "Execute")) || 
	    (streq_any (txt_fields[TYPE_TXT], "option", "option block", NULL) && 
	     streq (menu_element_txt_parent, "Execute"))) {
	  if (!streq (txt_fields[MENU_ELEMENT_TXT], "Execute"))
	    gtk_tree_model_iter_parent (model, &parent, &iter);
	  else
	    parent = iter;
	  if (gtk_tree_model_iter_n_children (model, &parent) < NUMBER_OF_EXECUTE_OPTS) {
	    if (streq (txt_fields[TYPE_TXT], "action") || 
		(streq (txt_fields[TYPE_TXT], "option block") && 
		 gtk_tree_model_iter_n_children (model, &iter) < NUMBER_OF_STARTUPNOTIFY_OPTS)) {
	      create_cm_headline (context_menu, 
				  (gtk_tree_model_iter_n_children (model, &parent) == NUMBER_OF_EXECUTE_OPTS - 1) ? 
				  " Execute option" : " Execute options");
	    }

	    add_startupnotify_or_execute_options_to_context_menu (context_menu, FALSE, &parent, NUMBER_OF_EXECUTE_OPTS, 
								  execute_options);

	    // If "Execute" is selected, the headline for the actions follows.
	    if (!streq (txt_fields[MENU_ELEMENT_TXT], "Execute"))
	      gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
	  }
	}
	// Cleanup
	g_free (menu_element_txt_parent);

	// Option of "Exit"/"SessionLogout" (prompt) or "Restart" action (command)
	if (streq (txt_fields[TYPE_TXT], "action") && 
	    streq_any (txt_fields[MENU_ELEMENT_TXT], "Exit", "SessionLogout", "Restart", NULL) && 
	    !gtk_tree_model_iter_has_child (model, &iter)) {
	  gboolean restart = (streq (txt_fields[MENU_ELEMENT_TXT], "Restart"));
	  create_cm_headline (context_menu, (streq (txt_fields[MENU_ELEMENT_TXT], "Exit")) ? " Exit option" : 
			      ((restart) ? " Restart option" : " SessionLogout option"));
	  menu_item = gtk_menu_item_new_with_label ((restart) ? "Add command" : "Add prompt");
	  g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (add_new), (restart) ? "command" : "prompt");
	  gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
	}
      }

      // Actions
      if (streq_any (txt_fields[TYPE_TXT], "item", "action", NULL)) {
	guint8 actions_cnt;

	if (streq (txt_fields[TYPE_TXT], "item") || 
	    ((streq (txt_fields[MENU_ELEMENT_TXT], "Execute") && 
	      gtk_tree_model_iter_n_children (model, &iter) < NUMBER_OF_EXECUTE_OPTS) ||
	     ((streq_any (txt_fields[MENU_ELEMENT_TXT], "Exit", "Restart", "SessionLogout", NULL) && 
	       gtk_tree_model_iter_n_children (model, &iter) == 0))))
	  create_cm_headline (context_menu, " Actions");
	for (actions_cnt = 0; actions_cnt < NUMBER_OF_ACTIONS; actions_cnt++) {
	  menu_item_txt = g_strconcat ("Add ", actions[actions_cnt], NULL);
	  menu_item = gtk_menu_item_new_with_label (menu_item_txt);
	  // Cleanup
	  g_free (menu_item_txt);

	  if (actions_cnt != RECONFIGURE)
	    g_signal_connect_swapped (menu_item, "activate", 
				    G_CALLBACK (generate_action_option_combo_box), actions[actions_cnt]);
	  else
	    g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (action_option_insert), "context menu");
	  gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
	}
	gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
      }
    }
  }

  // Basic menu elements

  /* menu, pipe menu, item & separator -> if (txt_fields[ELEMENT_VISIBILITY_TXT]) == TRUE
     If number of rows is > 1, then txt_fields[ELEMENT_VISIBILITY_TXT] == NULL. */
  if (!path || txt_fields[ELEMENT_VISIBILITY_TXT]) {
    gchar *basic_menu_elements[] = { "menu", "pipe menu", "item", "separator" };
    const gint NUMBER_OF_BASIC_MENU_ELEMENTS = G_N_ELEMENTS (basic_menu_elements);

    guint8 basic_menu_elements_cnt;

    if (!path) {
      gtk_tree_selection_unselect_all (selection);
      create_cm_headline (context_menu, " Add at toplevel");
    }
    for (basic_menu_elements_cnt = 0; 
	 basic_menu_elements_cnt < NUMBER_OF_BASIC_MENU_ELEMENTS; 
	 basic_menu_elements_cnt++) {
      menu_item_txt = g_strconcat ((!path) ? "" : "Add ", basic_menu_elements[basic_menu_elements_cnt], NULL);
      menu_item = gtk_menu_item_new_with_label (menu_item_txt);
      // Cleanup
      g_free (menu_item_txt);
 
      g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (add_new), 
				basic_menu_elements[basic_menu_elements_cnt]);
      gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
    }
    if (path)
      gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
  }

  if (path) {
    // Defaults
    gboolean invalid_row_for_change_of_element_visibility_exists = FALSE;
    gboolean at_least_one_descendant_is_invisible = FALSE;
    gboolean at_least_one_selected_row_has_no_children = FALSE;
    gboolean at_least_one_selected_row_is_expanded = FALSE;
    gboolean at_least_one_selected_row_is_not_expanded = FALSE;

    struct expansion_status_data expansion_status_of_subnodes = { 0 }; // All elements are initialised with "false".

    GtkTreeModel *filter_model;

    gchar *element_visibility_txt_loop;
    GtkTreePath *path_loop;

    // Visualisation of invisible menus, pipe menus, items and separators.
    for (g_list_loop = selected_rows; g_list_loop; g_list_loop = g_list_loop->next) {
      path_loop = g_list_loop->data;
      gtk_tree_model_get_iter (model, &iter_loop, path_loop);
      gtk_tree_model_get (model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_visibility_txt_loop, -1);
      if (!element_visibility_txt_loop || streq (element_visibility_txt_loop, "visible")) {
	invalid_row_for_change_of_element_visibility_exists = TRUE;

	// Cleanup
	g_free (element_visibility_txt_loop);

	break;
      }
      else if (gtk_tree_model_iter_has_child (model, &iter_loop)) {
	filter_model = gtk_tree_model_filter_new (model, path_loop);
	gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) check_if_invisible_descendant_exists, 
				&at_least_one_descendant_is_invisible);
      }

      // Cleanup
      g_free (element_visibility_txt_loop);
    }

    if (!invalid_row_for_change_of_element_visibility_exists) {
      for (guint8 visualisation_cnt = 0; visualisation_cnt < 2; visualisation_cnt++) {
	if (!(visualisation_cnt == 1 && !at_least_one_descendant_is_invisible)) {
	  menu_item = gtk_menu_item_new_with_label ((visualisation_cnt == 0) ? "Visualise" : "Visualise recursively");
	  g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (visualise_menus_items_and_separators), 
				    GUINT_TO_POINTER (visualisation_cnt)); // recursively = 1 = TRUE.
	  gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
	}
      }

      gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());
    }

    // Remove
    menu_item = gtk_menu_item_new_with_label ("Remove");
    gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
    g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (remove_rows), "context menu");

    for (g_list_loop = selected_rows; g_list_loop; g_list_loop = g_list_loop->next) {
      path_loop = g_list_loop->data;
      gtk_tree_model_get_iter (model, &iter_loop, path_loop);
      if (!gtk_tree_model_iter_has_child (model, &iter_loop)) {
	at_least_one_selected_row_has_no_children = TRUE;
	break;
      }
      else {
	GtkTreeModel *filter_model = gtk_tree_model_filter_new (model, path_loop);
 
	gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) check_expansion_status_of_subnodes, 
				&expansion_status_of_subnodes);

	if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), path_loop))
	  at_least_one_selected_row_is_expanded = TRUE;
	else
	  at_least_one_selected_row_is_not_expanded = TRUE;
      }
    }

    if (!at_least_one_selected_row_has_no_children) {
      // Remove all children
      menu_item = gtk_menu_item_new_with_label ("Remove all children");
      g_signal_connect (menu_item, "activate", G_CALLBACK (remove_all_children), NULL);
      gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);

      // Expand or collapse node(s)
      gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), gtk_separator_menu_item_new ());

      for (guint8 exp_status_changes_cnt = 0; 
	   exp_status_changes_cnt < NUMBER_OF_EXPANSION_STATUS_CHANGES; 
	   exp_status_changes_cnt++) {
	if ((exp_status_changes_cnt == RECURSIVELY && expansion_status_of_subnodes.at_least_one_is_collapsed) || 
	    (exp_status_changes_cnt == IMMEDIATE && 
	     (at_least_one_selected_row_is_not_expanded || expansion_status_of_subnodes.at_least_one_imd_ch_is_exp)) || 
	    (exp_status_changes_cnt == COLLAPSE && at_least_one_selected_row_is_expanded)) {
	  menu_item_txt = g_strdup_printf ((exp_status_changes_cnt == RECURSIVELY) ? "Expand row%s recursively" : 
					   ((exp_status_changes_cnt == IMMEDIATE) ? "Expand row%s (imd. ch. only)" : 
					    "Collapse row%s"), (number_of_selected_rows == 1) ? "" : "s");
	  menu_item = gtk_menu_item_new_with_label (menu_item_txt);
	  // Cleanup
	  g_free (menu_item_txt);

	  g_signal_connect_swapped (menu_item, "activate", G_CALLBACK (expand_or_collapse_selected_rows), 
				    GUINT_TO_POINTER (exp_status_changes_cnt));
	  gtk_menu_shell_append (GTK_MENU_SHELL (context_menu), menu_item);
	}
      }
    }
  }

  // Cleanup
  gtk_tree_path_free (path);
  g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);

  gtk_widget_show_all (context_menu);

  gtk_menu_popup (GTK_MENU (context_menu), NULL, NULL, NULL, NULL, (event) ? event->button : 0, 
		  gdk_event_get_time ((GdkEvent*) event));
}
