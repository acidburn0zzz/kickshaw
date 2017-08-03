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
#include <glib/gstdio.h>
#include <stdlib.h>

#include "general_header_files/enum__columns.h"
#include "general_header_files/enum__menu_bar_items.h"
#include "general_header_files/enum__toolbar_buttons.h"
#include "general_header_files/enum__ts_elements.h"
#include "general_header_files/enum__txt_fields.h"
#include "save_menu.h"

struct save_menu_args_data {
  FILE *menu_file;
  guint8 saving_stage;
  GtkTreeIter filter_iter[2];
  gint filter_path_depth_prev;
};

enum { MENUS, ROOT_MENU, IND_OF_MENU_STAGE };
enum { TOPLEVEL, FILTER_LEVEL, IND_OF_LEVEL };
enum { MENU_OR_PIPE_MENU, ITEM_OR_ACTION, SEPARATOR };
enum { CURRENT, PREV, NUMBER_OF_ITER_ARRAY_ELM };

static void closing_tags (gboolean last_row_reached, GtkTreeModel *filter_model, 
			  struct save_menu_args_data *save_menu_args);
static void write_tag (guint8 saving_stage, guint8 level, gchar **tag_elements, FILE *menu_file, 
		       GtkTreeModel *local_model, GtkTreeIter *local_iter, guint8 type);
static void get_field_values (gchar **txt_fields_array, GtkTreeModel *current_model, GtkTreeIter *current_iter);
static gboolean treestore_save_process_iteration (GtkTreeModel *filter_model, GtkTreePath *filter_path, 
						  GtkTreeIter *filter_iter, struct save_menu_args_data *save_menu_args);
static void process_menu_or_item (GtkTreeIter *process_iter, struct save_menu_args_data *save_menu_args);
void save_menu (gchar *save_as_filename);
void save_menu_as (void);

/* 

   Writes closing tags.

*/

static void closing_tags (gboolean                    last_row_reached, 
                          GtkTreeModel               *filter_model, 
			  struct save_menu_args_data *save_menu_args)
{
  FILE *menu_file = save_menu_args->menu_file;
  guint8 saving_stage = save_menu_args->saving_stage;
  GtkTreeIter *filter_iter = save_menu_args->filter_iter;
  gint filter_path_depth_prev = save_menu_args->filter_path_depth_prev;

  const guint8 offset = (saving_stage == MENUS); // TRUE = 1, FALSE = 0.

  gchar *ts_txt[NUMBER_OF_ITER_ARRAY_ELM][2]; // COL_MENU_ELEMENT & COL_TYPE for current and previous iter.

  /* 
     action_closing_tag_subtraction contains the number of indentations 
     that have to be removed for the closing action tag after the last option of an action.

     1 = option of action (='command' and 'prompt')
     <item label="Program">
       <action="Exeucte">
         <ommand>program</command>
       </action> = removed 1 in comparison to 'command'
     </item>
     <item="New item">
     ...

     2 = startupnotify option (='enabled', 'name', 'wmclass' and 'icon')
     <item label="Program">
       <action="Execute">
         <command>program</command>
         <startupnotify>
           <enabled>
         <startupnotify />
       </action> = removed 2 in comparison to 'enabled'
     <item>
     <item label="New item">
     ...

     The value is set to zero if the action is a self-closing action without an option (='Reconfigure')

     <item label="Reconfigure">
       <action="Reconfigure" /> = 0, self-closing action without an option
     </item> 
     <item label="New item"> 
     ...
  
     action_closing_tag_subtraction defaults to -1. This is done for the case that additional </menu> 
     closing tags have to be added. The reason for setting the default value to -1 is explained there.
  */
  gint8 action_closing_tag_subtraction = -1; // Default, 

  gint path_depth_prev_cnt, subtr_path_depth_prev_cnt;
  gint8 array_cnt;

  for (array_cnt = CURRENT; array_cnt < NUMBER_OF_ITER_ARRAY_ELM; array_cnt++)
    gtk_tree_model_get (filter_model, &filter_iter[array_cnt], 
			TS_MENU_ELEMENT, &ts_txt[array_cnt][COL_MENU_ELEMENT], 
			TS_TYPE, &ts_txt[array_cnt][COL_TYPE],
			-1);

  // startupnotify
  if ((streq (ts_txt[PREV][COL_TYPE], "option") && 
       streq_any (ts_txt[PREV][COL_MENU_ELEMENT], "enabled", "name", "wmclass", "icon", NULL)) &&
      (!(streq (ts_txt[CURRENT][COL_TYPE], "option") && 
	 streq_any (ts_txt[CURRENT][COL_MENU_ELEMENT], "enabled", "name", "wmclass", "icon", NULL))
       || last_row_reached)) {
    for (path_depth_prev_cnt = offset; path_depth_prev_cnt < filter_path_depth_prev; path_depth_prev_cnt++)
      fputs ("  ", menu_file);
    fputs ("</startupnotify>\n", menu_file);
  }

  // action
  if (streq_any (ts_txt[PREV][COL_TYPE], "option", "option block", NULL) &&
      (!streq_any (ts_txt[CURRENT][COL_TYPE], "option", "option block", NULL) || last_row_reached)) {
    action_closing_tag_subtraction = (streq_any (ts_txt[PREV][COL_MENU_ELEMENT], // 2 if startupnotify option.
						 "command", "prompt", "startupnotify", NULL)) ? 1 : 2;
    for (subtr_path_depth_prev_cnt = offset; 
	 subtr_path_depth_prev_cnt <= filter_path_depth_prev - action_closing_tag_subtraction;
	 subtr_path_depth_prev_cnt++)
      fputs ("  ", menu_file);
    fputs ("</action>\n", menu_file);
  }

  // action without option
  if (streq (ts_txt[PREV][COL_TYPE], "action") && !gtk_tree_model_iter_has_child (filter_model, &filter_iter[PREV]))
    action_closing_tag_subtraction = 0;

  // item
  if (!(streq_any (ts_txt[PREV][COL_TYPE], "menu", "pipe menu", "separator", NULL) ||
	(streq (ts_txt[PREV][COL_TYPE], "item") && 
	 !gtk_tree_model_iter_has_child (filter_model, &filter_iter[PREV]))) && 
      (streq_any (ts_txt[CURRENT][COL_TYPE], "menu", "pipe menu", "item", "separator", NULL) || last_row_reached)) {
    /* 
       If an open action has been closed (or it has been a self-closing action) and no new action follows, 
       an </item> tag is written.
       The calculation of the subtraction for this tag is  
       filter_path_depth_prev - action_closing_tag_subtraction - 1, 
       since the </item> closing tag is one indentation level below the one of the </action> closing tag:
       Either 
           <prompt>yes</prompt> (example)
         </action>
       </item>
       or
             <enabled>yes</enabled> (example)
           </startupnotify>
         </action>
       </item> 
    */
    for (subtr_path_depth_prev_cnt = offset; 
	 subtr_path_depth_prev_cnt <= filter_path_depth_prev - action_closing_tag_subtraction - 1;
	 subtr_path_depth_prev_cnt++)
      fputs ("  ", menu_file);
    fputs ("</item>\n", menu_file);
  }

  // Menu
  if (saving_stage == MENUS) {
    GtkTreePath *filter_path = gtk_tree_model_get_path (filter_model, &filter_iter[CURRENT]);
    const gint filter_path_depth = (last_row_reached) ? 1 : gtk_tree_path_get_depth (filter_path);

    // Cleanup
    gtk_tree_path_free (filter_path);

    /* 
       If the current element is a menu, pipe menu, item or separator and the current path depth is lower than 
       the previous one, addtional </menu> tags have to be added, as for example in these cases:

       Case 1:

                                                                Depth:  
       <menu id="menu level 1a" label="menu level 1a">          1
         <menu id="menu level 2" label="menu level 2">          2
	   <menu id="menu level 3" label="menu level 3">        3
	     <item label="1" />                                 4
	     <item label="2">                                   4
	       <action="Execute">                               5
	         <command>                                      6
		 <startupnotify>                                6
		   <enabled>no</enabled>                        7 previous element
		 </startupnotify>                               6
	       </action>                                        5
	     </item>                                            4

	   </menu>                                              3
         </menu>                                                2 
       </menu>                                                  1
       <menu id="menu level 1b">                                1 current element
       ...

       1 < filter_path_depth_prev (=7, 'enabled') - action_closing_tag_subtraction (here: 2) - 1 -> 1 < 4
       4 - 1 = 3 menu closing tags to write.
       
       Case 2:

                                                                Depth:
       <menu id="menu level 1a" label="menu level 1a">          1
         <menu id="menu level 1a" label="menu level 1a">        2
           <separator />                                        3 previous element
       
	 </menu> has to be added                                2
         <menu id="menu lebel 1b"> Level 2                      2 current element
         ...

       2 < filter_path_depth_prev (=3, 'separator') - action_closing_tag_substraction (here: -1) - 1 -> 2 < 3
       3 - 2 = 1 closing tag to write.

       If the current element is a menu/pipe menu/item/separator, its path depth is compared to the one of the 
       previous element. If the path depth of the current element is lower than the one of the previous element, 
       there are still open menu tags and additional </menu> tags have to be written prior to writing the tag for 
       the current element. 
       If the previous element is an option of an action or a self-closing action, the value of the previous path depth 
       has to be adjusted to the additional closing tags resulting from this, since there have to be written less 
       </menu> closing tags then. 
       For this case the action_closing_tag_substraction has been set above, and considering there is also a closing 
       item tag following afterwards, the calculation is 
       filter_path_depth_prev - action_closing_tag_subtraction - 1 
       (see above for the values for action_closing_tag subtraction). 
       If the previous element is not an option of an action or self-closing action, 
       but a self-closing menu/item, a pipe menu or separator, there is not only no action closing tag subtraction, 
       but the additional reduction for the closing item tag has also to be taken to account. 
       That is why action_closing_tag_subtraction defaults to -1 for this case, since this means  
       filter_path_depth_prev - (-1) - 1 = filter_path_depth_prev
       so this removes any reductions that would be caused by additional closing tags.
    */
    if ((streq_any (ts_txt[CURRENT][COL_TYPE], "menu", "pipe menu", "item", "separator", NULL) || last_row_reached) && 
	filter_path_depth < filter_path_depth_prev - action_closing_tag_subtraction - 1) {
      /* 
	 The closing menu tags are written for each path depth from 
	 filter_path_depth_prev - action_closing_tag_subtraction - 2
	 down to
	 filter_path_depth 
      */
      gint path_depth_of_closing_menu_tag = filter_path_depth_prev - action_closing_tag_subtraction - 2;
      gint menu_closing_tags_cnt;

      while (path_depth_of_closing_menu_tag >= filter_path_depth) {
	for (menu_closing_tags_cnt = 1; 
	     menu_closing_tags_cnt <= path_depth_of_closing_menu_tag; 
	     menu_closing_tags_cnt++)
	  fputs ("  ", menu_file);
	fputs ("</menu>\n", menu_file);
	path_depth_of_closing_menu_tag--;
      }
    }
  }
  
  // Cleanup
  for (array_cnt = CURRENT; array_cnt < NUMBER_OF_ITER_ARRAY_ELM; array_cnt++) {
    g_free (ts_txt[array_cnt][COL_MENU_ELEMENT]);
    g_free (ts_txt[array_cnt][COL_TYPE]);
  }
}

/* 

   Writes an (opening) menu, item, separator or action tag into the menu XML file.

*/

static void write_tag (guint8         saving_stage, 
                       guint8         level, 
                       gchar        **tag_elements, 
                       FILE          *menu_file, 
		       GtkTreeModel  *local_model, 
                       GtkTreeIter   *local_iter, 
                       guint8         type) 
{
  /* For actions and options there is no distinction made between MENUS and ROOT_MENU, 
     the value is always IND_OF_MENU_STAGE. */
  GString *file_string = g_string_new ((saving_stage == ROOT_MENU) ? "  " : "");

  if (type == MENU_OR_PIPE_MENU || type == ITEM_OR_ACTION) {
    if (type == MENU_OR_PIPE_MENU) {
      g_string_append_printf (file_string, "<menu id=\"%s\"", tag_elements[MENU_ID_TXT]);
      if (tag_elements[MENU_ELEMENT_TXT] && 
	  !(saving_stage == ROOT_MENU && streq (tag_elements[TYPE_TXT], "menu")))
	g_string_append_printf (file_string, " label=\"%s\"", tag_elements[MENU_ELEMENT_TXT]);
      if (streq (tag_elements[TYPE_TXT], "pipe menu"))
	g_string_append_printf (file_string, " execute=\"%s\"", 
				(tag_elements[EXECUTE_TXT]) ? (tag_elements[EXECUTE_TXT]) : "");
      if (tag_elements[ICON_PATH_TXT] && (saving_stage == ROOT_MENU || level == FILTER_LEVEL)) // icon="" is saved back.
	g_string_append_printf (file_string, " icon=\"%s\"", tag_elements[ICON_PATH_TXT]);
    }
    else if (type == ITEM_OR_ACTION) { // Item or action
      g_string_append (file_string, (streq (tag_elements[TYPE_TXT], "item")) ? "<item" : "<action name");
      if (streq (tag_elements[TYPE_TXT], "item")) {
	if (tag_elements[MENU_ELEMENT_TXT])
	  g_string_append_printf (file_string, " label=\"%s\"", tag_elements[MENU_ELEMENT_TXT]);
	if (tag_elements[ICON_PATH_TXT]) // icon="" is saved back.
	  g_string_append_printf (file_string, " icon=\"%s\"", tag_elements[ICON_PATH_TXT]);
      }
      else // Action
	g_string_append_printf (file_string, "=\"%s\"", tag_elements[MENU_ELEMENT_TXT]);
    }
    g_string_append_printf (file_string, "%s>\n", 
			    ((saving_stage == ROOT_MENU && type == MENU_OR_PIPE_MENU) || 
			     !gtk_tree_model_iter_has_child (local_model, local_iter)) ? "/" : "");
  }
  else { // Separator
    if (tag_elements[MENU_ELEMENT_TXT])
      g_string_append_printf (file_string, "<separator label=\"%s\"/>\n", tag_elements[MENU_ELEMENT_TXT]);
    else
      g_string_append (file_string, "<separator/>\n");
  }
    
  fputs (file_string->str, menu_file);

  // Cleanup
  g_string_free (file_string, TRUE);
}

/* 

   Gets all values of a row needed for saving and escapes their special characters.

*/

static void get_field_values (gchar **txt_fields_array, GtkTreeModel *current_model, GtkTreeIter *current_iter)
{
  guint8 txt_cnt;
  gchar *unescaped_save_txt;

  for (txt_cnt = 0; txt_cnt < NUMBER_OF_TXT_FIELDS; txt_cnt++) {
    gtk_tree_model_get (current_model, current_iter, txt_cnt + TS_ICON_PATH, &unescaped_save_txt, -1);
    txt_fields_array[txt_cnt] = (unescaped_save_txt) ? g_markup_escape_text (unescaped_save_txt, -1) : NULL;

    // Cleanup
    g_free (unescaped_save_txt);
  }
}

/* 

   Iterates through the elements of a menu or item for processing 
   of each node and generating lines for the xml file.

*/

static gboolean treestore_save_process_iteration (GtkTreeModel               *filter_model, 
						  GtkTreePath                *filter_path, 
						  GtkTreeIter                *filter_iter, 
						  struct save_menu_args_data *save_menu_args)
{
  gint filter_path_depth = gtk_tree_path_get_depth (filter_path);
  save_menu_args->filter_iter[CURRENT] = *filter_iter;

  // Write closing tag(s).
  if (filter_path_depth < save_menu_args->filter_path_depth_prev)
    closing_tags (FALSE, filter_model, save_menu_args);

  gchar *save_txts_filter[NUMBER_OF_TXT_FIELDS];
  FILE *menu_file = save_menu_args->menu_file;

  gint path_depth_cnt;

  get_field_values (save_txts_filter, filter_model, filter_iter);

  // Create leading whitespace for indenting.
  for (path_depth_cnt = (save_menu_args->saving_stage == MENUS); // TRUE = 1, FALSE = 0.
       path_depth_cnt <= filter_path_depth ;
       path_depth_cnt++)
    fputs ("  ", menu_file);

  // High-level and standard elements
  if (streq_any (save_txts_filter[TYPE_TXT], "menu", "pipe menu", NULL))
    write_tag (MENUS, FILTER_LEVEL, save_txts_filter, menu_file, filter_model, filter_iter, MENU_OR_PIPE_MENU);
  else if (streq_any (save_txts_filter[TYPE_TXT], "item", "action", "separator", NULL))
    write_tag (IND_OF_MENU_STAGE, IND_OF_LEVEL, save_txts_filter, menu_file, filter_model, filter_iter, 
	       streq (save_txts_filter[TYPE_TXT], "separator") ? SEPARATOR : ITEM_OR_ACTION);
  /* "Execute" options incl. "startupnotify" and its options 
     ("prompt" also for "Exit/SessionLogout" action, "command" also for "Restart" action). */
  else {
    if (save_txts_filter[VALUE_TXT] || gtk_tree_model_iter_has_child (filter_model, filter_iter)) {
      if (streq (save_txts_filter[TYPE_TXT], "option"))
	fprintf (menu_file, "<%s>%s</%s>\n", save_txts_filter[MENU_ELEMENT_TXT], save_txts_filter[VALUE_TXT], 
		 save_txts_filter[MENU_ELEMENT_TXT]);
      else
	fprintf (menu_file, "<startupnotify>\n");
    }
    else
      fprintf (menu_file, "<%s/>\n", save_txts_filter[MENU_ELEMENT_TXT]);
  }

  save_menu_args->filter_path_depth_prev = filter_path_depth;
  save_menu_args->filter_iter[PREV] = *filter_iter;

  // Cleanup
  free_elements_of_static_string_array (save_txts_filter, NUMBER_OF_TXT_FIELDS, FALSE);

  return FALSE;
}

/* 

   Processes all subrows of a menu or item.

*/

static void process_menu_or_item (GtkTreeIter                *process_iter, 
				  struct save_menu_args_data *save_menu_args) 
{
  GtkTreePath *path = gtk_tree_model_get_path (model, process_iter);
  GtkTreeModel *filter_model = gtk_tree_model_filter_new (model, path);

  // Cleanup
  gtk_tree_path_free (path);

  gtk_tree_model_foreach (filter_model, (GtkTreeModelForeachFunc) treestore_save_process_iteration, save_menu_args);
  closing_tags (TRUE, filter_model, save_menu_args);
  save_menu_args->filter_path_depth_prev = 0; // Reset
}

/* 

   Saves the currently edited menu.

*/

void save_menu (gchar *save_as_filename)
{
  GtkTreeIter save_menu_iter;
  gboolean valid;

  gchar *preliminary_filename = (save_as_filename) ? save_as_filename : filename;
  FILE *menu_file;

  gchar *save_txts_toplevel[NUMBER_OF_TXT_FIELDS];

  // Create a backup of the menu file if another one already exists with the same name.
  if (g_file_test (preliminary_filename, G_FILE_TEST_EXISTS)) {
    gchar *backup_file_name = g_strconcat (preliminary_filename, "~", NULL);

    if (g_rename (preliminary_filename, backup_file_name) != 0) {
      show_errmsg ("Could not create a backup file of the overwritten menu!");

      // Cleanup
      g_free (save_as_filename); // If save_menu is called directly, save_as_filename is NULL.
      g_free (backup_file_name);

      return;
    }
    
    // Cleanup
    g_free (backup_file_name);
  }

  // Open menu file.
  if (!(menu_file = fopen (preliminary_filename, "w"))) {
    show_errmsg ("Could not open menu file for writing!");

    // Cleanup
    g_free (save_as_filename); // If save_menu is called directly, save_as_filename is NULL.

    return;
  }

  if (save_as_filename)
    set_filename_and_window_title (save_as_filename);


  // --- Write menu file. ---


  gchar *standard_file_path = g_strconcat (getenv ("HOME"), "/.config/openbox/menu.xml", NULL);

  struct save_menu_args_data save_menu_args = {
    .menu_file = menu_file,
    .saving_stage = MENUS,
    .filter_path_depth_prev = 0
  };

  fputs ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n<openbox_menu>\n\n", menu_file);

  // Menus
  valid = gtk_tree_model_get_iter_first (model, &save_menu_iter);
  while (valid) {
    get_field_values (save_txts_toplevel, model, &save_menu_iter);

    if (streq (save_txts_toplevel[TYPE_TXT], "menu") || 
	(streq (save_txts_toplevel[TYPE_TXT], "pipe menu") && 
	 streq (save_txts_toplevel[ELEMENT_VISIBILITY_TXT], "invisible unintegrated menu"))) {
      write_tag (MENUS, TOPLEVEL, save_txts_toplevel, menu_file, model, &save_menu_iter, MENU_OR_PIPE_MENU);
      if (gtk_tree_model_iter_has_child (model, &save_menu_iter)) {
	process_menu_or_item (&save_menu_iter, &save_menu_args);
	fputs ("</menu>\n", menu_file);
      }
    }
    valid = gtk_tree_model_iter_next (model, &save_menu_iter);

    // Cleanup
    free_elements_of_static_string_array (save_txts_toplevel, NUMBER_OF_TXT_FIELDS, FALSE);
  }

  // Root menu
  fputs ("\n<menu id=\"root-menu\" label=\"Openbox 3\">\n", menu_file);

  save_menu_args.saving_stage = ROOT_MENU;

  valid = gtk_tree_model_get_iter_first (model, &save_menu_iter);
  while (valid) {
    get_field_values (save_txts_toplevel, model, &save_menu_iter);

    if (streq_any (save_txts_toplevel[TYPE_TXT], "menu", "pipe menu", NULL) && 
	!streq (save_txts_toplevel[ELEMENT_VISIBILITY_TXT], "invisible unintegrated menu"))
      write_tag (ROOT_MENU, TOPLEVEL, save_txts_toplevel, menu_file, model, &save_menu_iter, MENU_OR_PIPE_MENU);
    else if (streq_any (save_txts_toplevel[TYPE_TXT], "item", "separator", NULL)) {
      write_tag (ROOT_MENU, IND_OF_LEVEL, save_txts_toplevel, menu_file, model, &save_menu_iter, 
		 streq (save_txts_toplevel[TYPE_TXT], "item") ? ITEM_OR_ACTION : SEPARATOR);
      if (gtk_tree_model_iter_has_child (model, &save_menu_iter)) // = item with content.
	process_menu_or_item (&save_menu_iter, &save_menu_args);
    }

    valid = gtk_tree_model_iter_next (model, &save_menu_iter);

    // Cleanup
    free_elements_of_static_string_array (save_txts_toplevel, NUMBER_OF_TXT_FIELDS, FALSE);
  }

  fputs ("</menu>\n\n</openbox_menu>", menu_file);

  fclose (menu_file);
  change_done = FALSE;
  gtk_widget_set_sensitive (GTK_WIDGET (mb_file_menu_items[MB_SAVE]), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (tb[TB_SAVE]), FALSE);

  // "openbox --reconfigure" is only called when kickshaw is used under openbox. 
  if (streq (filename, standard_file_path) && 
      (system ("pgrep 'openbox' > /dev/null 2>&1")) == 0)
    if ((system ("openbox --reconfigure")) != 0)
      show_errmsg ("The menu was saved, but reconfiguration of Openbox failed.");

  // Cleanup
  g_free (standard_file_path);
}

/* 

   Asks for a file name prior to saving.

*/

void save_menu_as (void)
{
  GtkWidget *dialog;
  gchar *new_filename;

  create_file_dialog (&dialog, "Save Openbox Menu File As ...");

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    new_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (dialog);

    save_menu (new_filename);
  }
  else
    gtk_widget_destroy (dialog);
}
