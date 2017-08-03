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
#include <string.h>

#include "general_header_files/enum__action_option_combo.h"
#include "general_header_files/enum__add_buttons.h"
#include "general_header_files/enum__columns.h"
#include "general_header_files/enum__entry_fields.h"
#include "general_header_files/enum__find_entry_row_buttons.h"
#include "general_header_files/enum__invalid_icon_imgs.h"
#include "general_header_files/enum__menu_bar_items.h"
#include "general_header_files/enum__move_row.h"
#include "general_header_files/enum__startupnotify_options.h"
#include "general_header_files/enum__toolbar_buttons.h"
#include "general_header_files/enum__ts_elements.h"
#include "general_header_files/enum__txt_fields.h"
#include "general_header_files/enum__view_and_options_menu_items.h"
#include "kickshaw.h"

#define free_and_reassign(string, new_value) { g_free (string); string = new_value; }
#define streq(string1, string2) (g_strcmp0 ((string1), (string2)) == 0)

gchar *actions[] = { "Execute", "Exit", "Reconfigure", "Restart", "SessionLogout" };
const guint8 NUMBER_OF_ACTIONS = G_N_ELEMENTS (actions);

gchar *execute_options[] = { "prompt", "command", "startupnotify" };
gchar *execute_displayed_txt[] = { "Prompt", "Command", "Startupnotify" };
const guint8 NUMBER_OF_EXECUTE_OPTS = G_N_ELEMENTS (execute_options);

gchar *startupnotify_options[] = { "enabled", "name", "wmclass", "icon" };
gchar *startupnotify_displayed_txt[] = { "Enabled", "Name", "WM_CLASS", "Icon" };
const guint8 NUMBER_OF_STARTUPNOTIFY_OPTS = G_N_ELEMENTS (startupnotify_options);

gboolean change_done; // = automatically FALSE
gboolean autosort_options; // = automatically FALSE
gchar *filename; // = automatically NULL

GtkWidget *window;
GtkTreeStore *treestore;
#define TREEVIEW_COLUMN_OFFSET NUMBER_OF_TS_ELEMENTS - NUMBER_OF_COLUMNS
GtkTreeModel *model;

GtkWidget *treeview;
GtkTreeViewColumn *columns[NUMBER_OF_COLUMNS];
enum { TXT_RENDERER, EXCL_TXT_RENDERER, PIXBUF_RENDERER, BOOL_RENDERER, NUMBER_OF_RENDERERS };
GtkCellRenderer *renderer[NUMBER_OF_RENDERERS];
gchar *column_header_txts[] = { "Menu Element", "Type", "Value", "Menu ID", "Execute", "Element Visibility" };
gchar *txt_fields[NUMBER_OF_TXT_FIELDS]; // = automatically NULL
GtkTreeIter iter;

 // = automatically NULL
GSList *menu_ids;
GSList *rows_with_icons;

GdkPixbuf *invalid_icon_imgs[2]; // = automatically NULL

GtkWidget *mb_file_menu_items[NUMBER_OF_FILE_MENU_ITEMS];
GtkWidget *mb_edit;
GtkWidget *mb_edit_menu_items[NUMBER_OF_EDIT_MENU_ITEMS];
GtkWidget *mb_search;
GtkWidget *mb_expand_all_nodes, *mb_collapse_all_nodes;
GtkWidget *mb_options;

GtkWidget *mb_view_and_options[NUMBER_OF_VIEW_AND_OPTIONS];

GtkToolItem *tb[NUMBER_OF_TB_BUTTONS];

GtkWidget *button_grid;
GtkWidget *add_image;
GtkWidget *bt_bar_label;
GtkWidget *bt_add[NUMBER_OF_ADD_BUTTONS];
GtkWidget *bt_add_action_option_label;

GtkListStore *action_option_combo_box_liststore;
GtkTreeModel *action_option_combo_box_model;
GtkWidget *action_option_grid;
GtkWidget *action_option, *action_option_done;
GtkWidget *options_grid, *suboptions_grid;
GtkWidget *options_command_label, *options_prompt_label, *options_check_button_label;
GtkWidget *options_command, *options_prompt, *options_check_button;
#define NUMBER_OF_SUBOPTIONS G_N_ELEMENTS (startupnotify_options)
GtkWidget *suboptions_labels[NUMBER_OF_SUBOPTIONS], *suboptions_fields[NUMBER_OF_SUBOPTIONS];

GtkWidget *find_grid;
GtkWidget *find_button_entry_row[NUMBER_OF_FIND_ENTRY_ROW_BUTTONS];
GtkWidget *find_entry;
GtkWidget *find_in_columns[NUMBER_OF_COLUMNS - 1], *find_in_all_columns;
GtkWidget *find_match_case, *find_regular_expression; 
const gchar *search_term_str = "";
GList *rows_with_found_occurrences; // = automatically NULL

GtkWidget *entry_grid;
GtkWidget *entry_labels[NUMBER_OF_ENTRY_FIELDS], *entry_fields[NUMBER_OF_ENTRY_FIELDS];
GtkWidget *icon_chooser, *remove_icon;

GSList *source_paths; // = automatically NULL

GtkWidget *statusbar;

guint font_size;

GtkTargetEntry enable_list[] = {
  { "STRING", GTK_TARGET_SAME_WIDGET, 0 },
};

gint handler_id_row_selected, handler_id_action_option_combo_box, 
  handler_id_action_option_button_clicked, handler_id_show_startupnotify_options;
gint handler_id_find_in_columns[NUMBER_OF_COLUMNS];

static void general_initialisiation (void);
static void add_button_content (GtkWidget *button, gchar *label_text);
void show_errmsg (gchar *errmsg_raw_txt);
GtkWidget *new_label_with_formattings (gchar *label_txt);
GtkWidget *create_dialog (GtkWidget **dialog, gchar *dialog_title, gchar *stock_id, gchar *button_txt_1, 
			  gchar *button_txt_2, gchar *button_txt_3, gchar *label_txt, gboolean show_immediately);
static gboolean selection_block_unblock (GtkTreeSelection G_GNUC_UNUSED *selection, GtkTreeModel G_GNUC_UNUSED *model,
					 GtkTreePath G_GNUC_UNUSED *path, 
					 gboolean G_GNUC_UNUSED path_currently_selected, gpointer block_state);
static gboolean mouse_pressed (GtkTreeView *treeview, GdkEventButton *event);
static gboolean mouse_released (void);
void get_toplevel_iter_from_path (GtkTreeIter *local_iter, GtkTreePath *local_path);
void check_for_existing_options (GtkTreeIter *parent, guint8 number_of_opts, 
				 gchar **options_array, gboolean *opts_exist);
gboolean check_if_invisible_descendant_exists (GtkTreeModel *filter_model, GtkTreePath G_GNUC_UNUSED *filter_path,
					       GtkTreeIter *filter_iter, 
					       gboolean *at_least_one_descendant_is_invisible);
gchar *check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *path);
static gboolean evaluate_match (const GMatchInfo *match_info, GString *result, gpointer data);
static void set_column_attributes (GtkTreeViewColumn G_GNUC_UNUSED *cell_column, GtkCellRenderer *txt_renderer,
				   GtkTreeModel *cell_model, GtkTreeIter *cell_iter, gpointer column_number_pointer);
void change_view_and_options (gpointer activated_menu_item_pointer);
void clear_global_static_data (void);
static void expand_or_collapse_all (gpointer expand_pointer);
static void about (void);
gboolean unsaved_changes (void);
void new_menu (void);
static void quit_program (void);
gchar *get_modified_date_for_icon (gchar *icon_path);
static gboolean add_icon_occurrence_to_list (GtkTreeModel G_GNUC_UNUSED *local_model, 
					     GtkTreePath *local_path, GtkTreeIter *local_iter);
void create_list_of_icon_occurrences (void);
void activate_change_done (void);
void write_settings (void);
void create_invalid_icon_imgs (void);

int main (int argc, char *argv[])
{
  // --- Startup checks --- 


  // ### Display program version. ###

  if (streq (argv[1], "--version")) {
    g_print ("Kickshaw 0.5 RC2\n");
    exit (EXIT_SUCCESS);
  }

  // ### Check if X is running. ###

  if (!g_getenv ("DISPLAY")) {
    g_print ("X is not running!\n");
    exit (EXIT_FAILURE);
  }

  /*

  Version check will be re-enabled if the program requires a certain mininum minor version of GTK 3 in the future.

  // ### Check for GTK version 3 ###

  if (gtk_major_version < 3 && gtk_minor_version < x) {
    g_print ("This program requires version 3.x of GTK\n");
    gtk_init (&argc, &argv);
    show_errmsg ("This program requires version 3.x of GTK");
    exit (EXIT_FAILURE);
  }

  */


  // --- Initialise GTK, create the GUI and set up everything else. ---


  gtk_init (&argc, &argv);

  general_initialisiation ();

  gtk_main ();
}

/* 

   Creates GUI and signals, also loads settings and standard menu file, if they exist.

*/

static void general_initialisiation (void)
{
  GtkWidget *main_grid;
  
  GtkWidget *menubar;
  GSList *element_visibility_menu_item_group = NULL, *grid_menu_item_group = NULL;
  GtkWidget *mb_file, *mb_filemenu, 
    *mb_editmenu,
    *mb_searchmenu, *mb_find, 
    *mb_view, *mb_viewmenu, *mb_show_element_visibility_column, *mb_show_element_visibility_column_submenu, 
    *mb_show_grid, *mb_show_grid_submenu, 
    *mb_optionsmenu,
    *mb_help, *mb_helpmenu, *mb_about;
  GtkWidget *mb_separator_view[4];

  GtkAccelGroup *accel_group = NULL;
  guint accel_key;
  GdkModifierType accel_mod;

  gchar *visibility_txts[] = { "Show Element Visibility column", "Keep highlighting", "Don't keep highlighting"};
  gchar *grid_txts[] = { "No grid lines", "Horizontal", "Vertical", "Both" };
  guint8 txts_cnt;

  GtkWidget *toolbar;
  gchar *button_IDs[] = { GTK_STOCK_NEW, GTK_STOCK_OPEN, GTK_STOCK_SAVE, GTK_STOCK_SAVE_AS, GTK_STOCK_GO_UP, 
  			  GTK_STOCK_GO_DOWN, GTK_STOCK_REMOVE, GTK_STOCK_FIND, GTK_STOCK_ZOOM_IN, GTK_STOCK_ZOOM_OUT, 
  			  GTK_STOCK_QUIT }; 
  gchar *tb_tooltips[] = { "New menu", "Open menu", "Save menu", "Save menu as...", "Move up", 
  			   "Move down", "Remove", "Find", "Expand all", "Collapse all", "Quit" };
  GtkToolItem *tb_separator;

  // Label text of the last button is set dynamically dependent on the type of the selected row.
  gchar *bt_add_txts[] = { "Menu", "Pipe Menu", "Item", "Separator", "" };

  GtkWidget *new_action_grid;
  
  gchar *find_buttons[] = { GTK_STOCK_CLOSE, GTK_STOCK_GO_BACK, GTK_STOCK_GO_FORWARD };
  gchar *find_buttons_tooltips[] = { "Close", "Back", "Forward" };
  GtkWidget *find_entry_row, *find_buttons_row, *find_special_options_row;
  GtkWidget *find_button_entry_row_image[NUMBER_OF_FIND_ENTRY_ROW_BUTTONS];
  
  GtkWidget *scrolled_window;
  GtkTreeSelection *selection;

  GtkCellRenderer *action_option_combo_box_renderer;
  GtkWidget *action_option_cancel;

  GtkWidget *icon_chooser_image, *remove_icon_image;
  
  gchar *suboptions_labels_txts[NUMBER_OF_STARTUPNOTIFY_OPTS];

  GKeyFile *settings_file = g_key_file_new ();
  GError *settings_file_error = NULL;
  gchar *settings_file_path;

  gchar *new_filename;

  guint8 buttons_cnt, columns_cnt, entry_fields_cnt, mb_menu_items_cnt, snotify_opts_cnt, view_and_opts_cnt;


  // --- Creating the GUI. ---


  // ### Create a new window. ###

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 650, 550);
  gtk_window_set_title (GTK_WINDOW (window), "Kickshaw");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  // ### Create a new grid that will contain all elements. ###

  main_grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (main_grid), TRUE);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (main_grid), GTK_ORIENTATION_VERTICAL);
  gtk_container_add (GTK_CONTAINER (window), main_grid);

  // ### Create a menu bar. ###

  menubar = gtk_menu_bar_new ();
  gtk_container_add (GTK_CONTAINER (main_grid), menubar);

  mb_filemenu = gtk_menu_new ();
  mb_editmenu = gtk_menu_new ();
  mb_searchmenu = gtk_menu_new ();
  mb_viewmenu = gtk_menu_new ();
  mb_optionsmenu = gtk_menu_new ();
  mb_helpmenu = gtk_menu_new ();

  // Submenus
  mb_show_element_visibility_column_submenu = gtk_menu_new ();
  mb_show_grid_submenu = gtk_menu_new ();

  // Accelerator Group
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  // File
  mb_file = gtk_menu_item_new_with_mnemonic ("_File");
  mb_file_menu_items[MB_NEW] = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, accel_group);
  mb_file_menu_items[MB_OPEN] = gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, accel_group);
  mb_file_menu_items[MB_SAVE] = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE, accel_group);
  mb_file_menu_items[MB_SAVE_AS] = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE_AS, accel_group);
  gtk_accelerator_parse ("<Shift><Ctl>S", &accel_key, &accel_mod);
  gtk_widget_add_accelerator (mb_file_menu_items[MB_SAVE_AS], "activate", accel_group, 
			      accel_key, accel_mod, GTK_ACCEL_VISIBLE);
  mb_file_menu_items[MB_SEPARATOR_FILE] = gtk_separator_menu_item_new ();
  mb_file_menu_items[MB_QUIT] = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, accel_group);

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_file), mb_filemenu);
  for (mb_menu_items_cnt = 0; mb_menu_items_cnt < NUMBER_OF_FILE_MENU_ITEMS; mb_menu_items_cnt++)
    gtk_menu_shell_append (GTK_MENU_SHELL (mb_filemenu), mb_file_menu_items[mb_menu_items_cnt]);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), mb_file);

  // Edit
  mb_edit = gtk_menu_item_new_with_mnemonic ("_Edit");
  mb_edit_menu_items[MB_MOVE_TOP] = gtk_image_menu_item_new_from_stock (GTK_STOCK_GOTO_TOP, NULL);
  mb_edit_menu_items[MB_MOVE_UP] = gtk_image_menu_item_new_from_stock (GTK_STOCK_GO_UP, NULL);
  mb_edit_menu_items[MB_MOVE_DOWN] = gtk_image_menu_item_new_from_stock (GTK_STOCK_GO_DOWN, NULL);
  mb_edit_menu_items[MB_MOVE_BOTTOM] = gtk_image_menu_item_new_from_stock (GTK_STOCK_GOTO_BOTTOM, NULL);
  mb_edit_menu_items[MB_SEPARATOR_EDIT1] = gtk_separator_menu_item_new ();
  mb_edit_menu_items[MB_REMOVE] = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
  mb_edit_menu_items[MB_REMOVE_ALL_CHILDREN] = gtk_menu_item_new_with_label ("Remove all children");
  mb_edit_menu_items[MB_SEPARATOR_EDIT2] = gtk_separator_menu_item_new ();
  mb_edit_menu_items[MB_VISUALISE] = gtk_menu_item_new_with_label ("Visualise");
  mb_edit_menu_items[MB_VISUALISE_RECURSIVELY] = gtk_menu_item_new_with_label ("Visualise recursively");

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_edit), mb_editmenu);
  for (mb_menu_items_cnt = 0; mb_menu_items_cnt < NUMBER_OF_EDIT_MENU_ITEMS; mb_menu_items_cnt++)
    gtk_menu_shell_append (GTK_MENU_SHELL (mb_editmenu), mb_edit_menu_items[mb_menu_items_cnt]);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), mb_edit);

  // Search
  mb_search = gtk_menu_item_new_with_mnemonic ("_Search");
  mb_find = gtk_image_menu_item_new_from_stock (GTK_STOCK_FIND, accel_group);

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_search), mb_searchmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_searchmenu), mb_find);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), mb_search);

  // View
  mb_view = gtk_menu_item_new_with_mnemonic ("_View");
  mb_expand_all_nodes = gtk_menu_item_new_with_label ("Expand all nodes");
  gtk_accelerator_parse ("<Ctl>X", &accel_key, &accel_mod);
  gtk_widget_add_accelerator (mb_expand_all_nodes, "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);
  mb_collapse_all_nodes = gtk_menu_item_new_with_label ("Collapse all nodes");
  gtk_accelerator_parse ("<Ctl>C", &accel_key, &accel_mod);
  gtk_widget_add_accelerator (mb_collapse_all_nodes, "activate", accel_group, accel_key, accel_mod, GTK_ACCEL_VISIBLE);
  mb_separator_view[0] = gtk_separator_menu_item_new ();
  mb_view_and_options[SHOW_MENU_ID_COL] = gtk_check_menu_item_new_with_label ("Show Menu ID column");
  mb_view_and_options[SHOW_EXECUTE_COL] = gtk_check_menu_item_new_with_label ("Show Execute column");
  mb_show_element_visibility_column = gtk_menu_item_new_with_label ("Show Element Visibility column");
  mb_separator_view[1] = gtk_separator_menu_item_new ();
  mb_view_and_options[SHOW_ICONS] = gtk_check_menu_item_new_with_label ("Show icons");
  mb_separator_view[2] = gtk_separator_menu_item_new ();
  mb_view_and_options[SHOW_SEP_IN_BOLD_TYPE] = gtk_check_menu_item_new_with_label ("Show separators in bold type");
  mb_separator_view[3] = gtk_separator_menu_item_new ();
  mb_view_and_options[DRAW_ROWS_IN_ALT_COLOURS] = 
    gtk_check_menu_item_new_with_label ("Draw rows in altern. colours (theme dep.)");
  mb_view_and_options[SHOW_TREE_LINES] = gtk_check_menu_item_new_with_label ("Show tree lines"); 
  mb_show_grid = gtk_menu_item_new_with_label ("Show grid");

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_show_element_visibility_column), 
			     mb_show_element_visibility_column_submenu);
  for (mb_menu_items_cnt = SHOW_ELEMENT_VISIBILITY_COL_ACT, txts_cnt = 0;
       mb_menu_items_cnt <= SHOW_ELEMENT_VISIBILITY_COL_DONT_KEEP_HIGHL; 
       mb_menu_items_cnt++, txts_cnt++) {
    if (mb_menu_items_cnt == SHOW_ELEMENT_VISIBILITY_COL_ACT)
      mb_view_and_options[mb_menu_items_cnt] = gtk_check_menu_item_new_with_label (visibility_txts[txts_cnt]);
    else {
      mb_view_and_options[mb_menu_items_cnt] = gtk_radio_menu_item_new_with_label (element_visibility_menu_item_group, 
										   visibility_txts[txts_cnt]);
      element_visibility_menu_item_group =
	gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (mb_view_and_options[mb_menu_items_cnt]));
    }
    gtk_menu_shell_append (GTK_MENU_SHELL (mb_show_element_visibility_column_submenu),
			   mb_view_and_options[mb_menu_items_cnt]);
  }

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_show_grid), mb_show_grid_submenu);
  for (mb_menu_items_cnt = NO_GRID_LINES, txts_cnt = 0; 
       mb_menu_items_cnt <= BOTH; 
       mb_menu_items_cnt++, txts_cnt++) {
    mb_view_and_options[mb_menu_items_cnt] = gtk_radio_menu_item_new_with_label (grid_menu_item_group, 
										 grid_txts[txts_cnt]);
    grid_menu_item_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (mb_view_and_options[mb_menu_items_cnt]));
    gtk_menu_shell_append (GTK_MENU_SHELL (mb_show_grid_submenu), mb_view_and_options[mb_menu_items_cnt]);
  }

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_view), mb_viewmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_expand_all_nodes);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_collapse_all_nodes);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_separator_view[0]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_view_and_options[SHOW_MENU_ID_COL]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_view_and_options[SHOW_EXECUTE_COL]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_show_element_visibility_column);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_separator_view[1]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_view_and_options[SHOW_ICONS]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_separator_view[2]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_view_and_options[SHOW_SEP_IN_BOLD_TYPE]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_separator_view[3]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_view_and_options[DRAW_ROWS_IN_ALT_COLOURS]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_view_and_options[SHOW_TREE_LINES]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_viewmenu), mb_show_grid);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), mb_view);
  
  // Default settings
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[SHOW_MENU_ID_COL]), TRUE);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[SHOW_EXECUTE_COL]), TRUE);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[SHOW_ICONS]), TRUE);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[SHOW_SEP_IN_BOLD_TYPE]), TRUE);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[SHOW_TREE_LINES]), TRUE);
  gtk_widget_set_sensitive (mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_KEEP_HIGHL], FALSE);
  gtk_widget_set_sensitive (mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_DONT_KEEP_HIGHL], FALSE);

  // Options
  mb_options = gtk_menu_item_new_with_mnemonic ("_Options");
  mb_view_and_options[SORT_EXECUTE_AND_STARTUPN_OPTIONS] = 
		      gtk_check_menu_item_new_with_label ("Sort execute/startupnotify options");
  mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS] = 
		      gtk_check_menu_item_new_with_label ("Always notify about execute opt. conversions");

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_options), mb_optionsmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_optionsmenu), mb_view_and_options[SORT_EXECUTE_AND_STARTUPN_OPTIONS]);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_optionsmenu), mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS]);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), mb_options);

  // Default settings
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS]), TRUE);

  // Help
  mb_help = gtk_menu_item_new_with_mnemonic ("_Help");
  mb_about = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mb_help), mb_helpmenu);
  gtk_menu_shell_append (GTK_MENU_SHELL (mb_helpmenu), mb_about);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), mb_help);

  // ### Create toolbar. ###

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH_HORIZ);
  gtk_container_add (GTK_CONTAINER (main_grid), toolbar);

  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 2);
  for (buttons_cnt = 0; buttons_cnt < NUMBER_OF_TB_BUTTONS; buttons_cnt++) {
    if (buttons_cnt == TB_MOVE_UP || buttons_cnt == TB_FIND || buttons_cnt == TB_QUIT) {
      tb_separator = gtk_separator_tool_item_new ();
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tb_separator, -1);
    }

    tb[buttons_cnt] = gtk_tool_button_new_from_stock (button_IDs[buttons_cnt]);
    gtk_widget_set_tooltip_text (GTK_WIDGET (tb[buttons_cnt]), tb_tooltips[buttons_cnt]);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tb[buttons_cnt], -1);
  }

  // ### Create Button Bar. ###

  button_grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (main_grid), button_grid);

  add_image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button_grid), add_image);

  bt_bar_label = gtk_label_new ("Add new:  ");
  gtk_container_add (GTK_CONTAINER (button_grid), bt_bar_label);

  // Label text is set dynamically dependent on the type of the selected row.
  bt_add_action_option_label = gtk_label_new ("");

  for (buttons_cnt = 0; buttons_cnt < NUMBER_OF_ADD_BUTTONS; buttons_cnt++) {
    bt_add[buttons_cnt] = gtk_button_new ();
    add_button_content (bt_add[buttons_cnt], bt_add_txts[buttons_cnt]);
    gtk_container_add (GTK_CONTAINER (button_grid), bt_add[buttons_cnt]);
  }

  // ### Grid for entering the values of new rows. ###

  action_option_grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (action_option_grid), TRUE);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (action_option_grid), GTK_ORIENTATION_VERTICAL);
  gtk_container_set_border_width (GTK_CONTAINER (action_option_grid), 5);
  gtk_container_add (GTK_CONTAINER (main_grid), action_option_grid);

  new_action_grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (action_option_grid), new_action_grid);

  // Action Combo Box
  action_option_combo_box_liststore = gtk_list_store_new (ACTION_OPTION_COMBO_COLS, G_TYPE_STRING);
  action_option_combo_box_model = GTK_TREE_MODEL (action_option_combo_box_liststore);
  action_option = gtk_combo_box_new_with_model (action_option_combo_box_model);
  action_option_combo_box_renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (action_option), action_option_combo_box_renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (action_option), action_option_combo_box_renderer, "text", 0, NULL);
  gtk_combo_box_set_id_column (GTK_COMBO_BOX (action_option), 0);

  g_object_unref (action_option_combo_box_liststore);

  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (action_option), action_option_combo_box_renderer, 
  				      option_list_with_headlines, NULL, NULL);

  gtk_container_add (GTK_CONTAINER (new_action_grid), action_option);

  action_option_done = gtk_button_new ();
  add_button_content (action_option_done, "Done");
  gtk_container_add (GTK_CONTAINER (new_action_grid), action_option_done);

  action_option_cancel = gtk_button_new ();
  add_button_content (action_option_cancel, "Cancel");
  gtk_container_add (GTK_CONTAINER (new_action_grid), action_option_cancel);

  // Execute options
  options_grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (action_option_grid), options_grid);

  options_command_label = gtk_label_new ("Command: ");
  gtk_misc_set_alignment (GTK_MISC (options_command_label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (options_grid), options_command_label, 0, 0, 2, 1);

  options_command = gtk_entry_new ();
  gtk_widget_set_hexpand (options_command, TRUE);
  gtk_grid_attach (GTK_GRID (options_grid), options_command, 2, 0, 1, 1);

  options_prompt_label = gtk_label_new ("Prompt (optional): ");
  gtk_misc_set_alignment (GTK_MISC (options_prompt_label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (options_grid), options_prompt_label, 0, 1, 2, 1);

  options_prompt = gtk_entry_new ();
  gtk_widget_set_hexpand (options_prompt, TRUE);
  gtk_grid_attach (GTK_GRID (options_grid), options_prompt, 2, 1, 1, 1);

  options_check_button_label = gtk_label_new ("Startupnotify ");
  gtk_grid_attach (GTK_GRID (options_grid), options_check_button_label, 0, 2, 1, 1);

  options_check_button = gtk_check_button_new ();
  gtk_grid_attach (GTK_GRID (options_grid), options_check_button, 1, 2, 1, 1);

  suboptions_grid = gtk_grid_new ();
  gtk_grid_attach (GTK_GRID (options_grid), suboptions_grid, 2, 2, 1, 1);

  for (snotify_opts_cnt = ENABLED; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++) {
    suboptions_labels_txts[snotify_opts_cnt] = g_strconcat (startupnotify_displayed_txt[snotify_opts_cnt], ": ", NULL);
    suboptions_labels[snotify_opts_cnt] = gtk_label_new (suboptions_labels_txts[snotify_opts_cnt]);

    // Cleanup
    g_free (suboptions_labels_txts[snotify_opts_cnt]);

    gtk_misc_set_alignment (GTK_MISC (suboptions_labels[snotify_opts_cnt]), 0.0, 0.5);
    gtk_grid_attach (GTK_GRID (suboptions_grid), suboptions_labels[snotify_opts_cnt], 0, snotify_opts_cnt, 1, 1);

    suboptions_fields[snotify_opts_cnt] = (snotify_opts_cnt == ENABLED) ? gtk_check_button_new () : gtk_entry_new ();
    gtk_grid_attach (GTK_GRID (suboptions_grid), suboptions_fields[snotify_opts_cnt], 1, snotify_opts_cnt, 1, 1);
    
    gtk_widget_set_hexpand (suboptions_fields[snotify_opts_cnt], TRUE);
  }

  // ### Create find grid, ###

  find_grid = gtk_grid_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (find_grid), GTK_ORIENTATION_VERTICAL);
  gtk_container_add (GTK_CONTAINER (main_grid), find_grid);

  find_entry_row = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (find_grid), find_entry_row);

  for (buttons_cnt = 0; buttons_cnt < NUMBER_OF_FIND_ENTRY_ROW_BUTTONS; buttons_cnt++) {
    find_button_entry_row_image[buttons_cnt] = gtk_image_new_from_stock (find_buttons[buttons_cnt], 
									 GTK_ICON_SIZE_BUTTON);
    find_button_entry_row[buttons_cnt] = gtk_button_new ();
    gtk_button_set_image (GTK_BUTTON (find_button_entry_row[buttons_cnt]), find_button_entry_row_image[buttons_cnt]);
    gtk_widget_set_tooltip_text (GTK_WIDGET (find_button_entry_row[buttons_cnt]), find_buttons_tooltips[buttons_cnt]);
    if (buttons_cnt > CLOSE)
      gtk_widget_set_sensitive (find_button_entry_row[buttons_cnt], FALSE);
    gtk_container_add (GTK_CONTAINER (find_entry_row), find_button_entry_row[buttons_cnt]);
  }

  find_entry = gtk_entry_new ();
  gtk_widget_set_hexpand (find_entry, TRUE);
  gtk_container_add (GTK_CONTAINER (find_entry_row), find_entry);
  
  find_buttons_row = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (find_grid), find_buttons_row);
  
  for (columns_cnt = 0; columns_cnt < NUMBER_OF_COLUMNS - 1; columns_cnt++) {
    find_in_columns[columns_cnt] = gtk_check_button_new_with_label (column_header_txts[columns_cnt]);
    gtk_container_add (GTK_CONTAINER (find_buttons_row), find_in_columns[columns_cnt]);
  }
  
  find_in_all_columns = gtk_check_button_new_with_label ("All columns");
  gtk_container_add (GTK_CONTAINER (find_buttons_row), find_in_all_columns);
  
  find_special_options_row = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (find_grid), find_special_options_row);
  
  find_match_case = gtk_check_button_new_with_label ("Match case");
  gtk_container_add (GTK_CONTAINER (find_special_options_row), find_match_case);
  
  find_regular_expression = gtk_check_button_new_with_label ("Regular expression");
  gtk_container_add (GTK_CONTAINER (find_special_options_row), find_regular_expression);

  // Default settings
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_regular_expression), TRUE);

  // ### Initialise Tree View. ###

  treeview = gtk_tree_view_new ();
  gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW (treeview), TRUE); // Default

  // Create columns with cell renderers and add them to the treeview.

  for (columns_cnt = 0; columns_cnt < NUMBER_OF_COLUMNS; columns_cnt++) {
    columns[columns_cnt] = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (columns[columns_cnt], column_header_txts[columns_cnt]);

    if (columns_cnt == COL_MENU_ELEMENT) {
      renderer[PIXBUF_RENDERER] = gtk_cell_renderer_pixbuf_new ();
      renderer[EXCL_TXT_RENDERER] = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (columns[COL_MENU_ELEMENT], renderer[PIXBUF_RENDERER], FALSE);
      gtk_tree_view_column_pack_start (columns[COL_MENU_ELEMENT], renderer[EXCL_TXT_RENDERER], FALSE);
      gtk_tree_view_column_set_attributes (columns[COL_MENU_ELEMENT], renderer[PIXBUF_RENDERER], 
					   "pixbuf", TS_ICON_IMG, NULL);
    }
    else if (columns_cnt == COL_VALUE) {
      renderer[BOOL_RENDERER] = gtk_cell_renderer_toggle_new ();
      g_signal_connect (renderer[BOOL_RENDERER], "toggled", G_CALLBACK (boolean_toogled), 
			column_header_txts[columns_cnt]);
      gtk_tree_view_column_pack_start (columns[COL_VALUE], renderer[BOOL_RENDERER], FALSE);
    }

    renderer[TXT_RENDERER] = gtk_cell_renderer_text_new ();
    g_signal_connect (renderer[TXT_RENDERER], "edited", G_CALLBACK (cell_edited), GUINT_TO_POINTER (columns_cnt));
    gtk_tree_view_column_pack_start (columns[columns_cnt], renderer[TXT_RENDERER], FALSE);
    gtk_tree_view_column_set_attributes (columns[columns_cnt], renderer[TXT_RENDERER], 
    					 "text", columns_cnt + TREEVIEW_COLUMN_OFFSET, NULL);

    gtk_tree_view_column_set_cell_data_func (columns[columns_cnt], renderer[TXT_RENDERER], 
    					     (GtkTreeCellDataFunc) set_column_attributes, 
    					     GUINT_TO_POINTER (columns_cnt), NULL);
    gtk_tree_view_column_set_resizable (columns[columns_cnt], TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), columns[columns_cnt]);
  }

  // Default settings
  gtk_tree_view_column_set_visible (columns[COL_ELEMENT_VISIBILITY], FALSE);

  // Set treestore and model.
  treestore = gtk_tree_store_new (NUMBER_OF_TS_ELEMENTS, GDK_TYPE_PIXBUF, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, 
				  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, 
				  G_TYPE_STRING);

  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (treestore));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  g_object_unref (treestore);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

  // Set scrolled window that contains the treeview.
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (main_grid), scrolled_window);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolled_window), treeview);
  gtk_widget_set_vexpand (scrolled_window, TRUE);

  // Set drag and drop destination parameters.
  gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview), enable_list, 1, GDK_ACTION_MOVE);

  // ### Create edit grid. ###

  entry_grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (main_grid), entry_grid);

  // Label text is set dynamically dependent on the type of the selected row.
  entry_labels[MENU_ELEMENT_OR_VALUE_ENTRY] = gtk_label_new (""); 
  gtk_misc_set_alignment (GTK_MISC (entry_labels[MENU_ELEMENT_OR_VALUE_ENTRY]), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (entry_grid), entry_labels[MENU_ELEMENT_OR_VALUE_ENTRY], 0, 0, 1, 1);

  entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY] = gtk_entry_new ();
  gtk_widget_set_hexpand (entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY], TRUE);
  gtk_grid_attach (GTK_GRID (entry_grid), entry_fields[MENU_ELEMENT_OR_VALUE_ENTRY], 1, 0, 1, 1);

  entry_labels[ICON_PATH_ENTRY] = gtk_label_new (" Icon: ");
  gtk_grid_attach (GTK_GRID (entry_grid), entry_labels[ICON_PATH_ENTRY], 2, 0, 1, 1);

  icon_chooser_image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
  icon_chooser = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (icon_chooser), icon_chooser_image);
  gtk_widget_set_tooltip_text (GTK_WIDGET (icon_chooser), "Add/Change icon");
  gtk_grid_attach (GTK_GRID (entry_grid), icon_chooser, 3, 0, 1, 1);

  remove_icon_image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON);
  remove_icon = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (remove_icon), remove_icon_image);
  gtk_widget_set_tooltip_text (GTK_WIDGET (remove_icon), "Remove icon");
  gtk_grid_attach (GTK_GRID (entry_grid), remove_icon, 4, 0, 1, 1);

  entry_fields[ICON_PATH_ENTRY] = gtk_entry_new ();
  gtk_widget_set_hexpand (entry_fields[ICON_PATH_ENTRY], TRUE);
  gtk_grid_attach (GTK_GRID (entry_grid), entry_fields[ICON_PATH_ENTRY], 5, 0, 1, 1);

  for (entry_fields_cnt = 2; entry_fields_cnt < NUMBER_OF_ENTRY_FIELDS; entry_fields_cnt++) {
    entry_labels[entry_fields_cnt] = gtk_label_new ((entry_fields_cnt == 2) ? " Menu ID: " : " Execute: ");
    gtk_grid_attach (GTK_GRID (entry_grid), entry_labels[entry_fields_cnt], 0, entry_fields_cnt - 1, 1, 1);
  
    entry_fields[entry_fields_cnt] = gtk_entry_new ();
    gtk_grid_attach (GTK_GRID (entry_grid), entry_fields[entry_fields_cnt], 1, entry_fields_cnt - 1, 5, 1);
  }
  
  // ### Statusbar ###

  statusbar = gtk_statusbar_new ();
  gtk_container_add (GTK_CONTAINER (main_grid), statusbar);

  // ### Get the default font size. ###
  font_size = get_font_size ();

  // ### Create broken icon image suitable for that font size.
  create_invalid_icon_imgs ();


  // --- Create signals for all buttons and relevant events. ---


  handler_id_row_selected = g_signal_connect (selection, "changed", G_CALLBACK (row_selected), NULL);
  g_signal_connect (treeview, "row-expanded", G_CALLBACK (set_status_of_expand_and_collapse_buttons_and_menu_items),
							  NULL);
  g_signal_connect (treeview, "row-collapsed", G_CALLBACK (set_status_of_expand_and_collapse_buttons_and_menu_items), 
							   NULL);
  g_signal_connect (treeview, "button-press-event", G_CALLBACK (mouse_pressed), NULL);
  g_signal_connect (treeview, "button-release-event", G_CALLBACK (mouse_released), NULL);

  g_signal_connect (treeview, "key-press-event", G_CALLBACK (key_pressed), NULL);

  g_signal_connect (treeview, "drag-motion", G_CALLBACK (drag_motion_handler), NULL);
  g_signal_connect (treeview, "drag_data_received", G_CALLBACK (drag_data_received_handler), NULL);

  g_signal_connect (mb_file_menu_items[MB_NEW], "activate", G_CALLBACK (new_menu), NULL);
  g_signal_connect (mb_file_menu_items[MB_OPEN], "activate", G_CALLBACK (open_menu), NULL);
  g_signal_connect_swapped (mb_file_menu_items[MB_SAVE], "activate", G_CALLBACK (save_menu), NULL);
  g_signal_connect (mb_file_menu_items[MB_SAVE_AS], "activate", G_CALLBACK (save_menu_as), NULL);
  g_signal_connect (mb_file_menu_items[MB_QUIT], "activate", G_CALLBACK (quit_program), NULL);

  for (mb_menu_items_cnt = TOP; mb_menu_items_cnt <= BOTTOM; mb_menu_items_cnt++) {
    g_signal_connect_swapped (mb_edit_menu_items[mb_menu_items_cnt], "activate", 
			      G_CALLBACK (move_selection), GUINT_TO_POINTER (mb_menu_items_cnt));
  }
  g_signal_connect_swapped (mb_edit_menu_items[MB_REMOVE], "activate", G_CALLBACK (remove_rows), "menu bar");
  g_signal_connect (mb_edit_menu_items[MB_REMOVE_ALL_CHILDREN], "activate", G_CALLBACK (remove_all_children), NULL);
  g_signal_connect_swapped (mb_edit_menu_items[MB_VISUALISE], "activate", 
			    G_CALLBACK (visualise_menus_items_and_separators), GUINT_TO_POINTER (FALSE));
  g_signal_connect_swapped (mb_edit_menu_items[MB_VISUALISE_RECURSIVELY], "activate", 
			    G_CALLBACK (visualise_menus_items_and_separators), GUINT_TO_POINTER (TRUE));

  g_signal_connect (mb_find, "activate", G_CALLBACK (show_or_hide_find_grid), NULL);

  g_signal_connect_swapped (mb_expand_all_nodes, "activate",
			    G_CALLBACK (expand_or_collapse_all), GUINT_TO_POINTER (TRUE));
  g_signal_connect_swapped (mb_collapse_all_nodes, "activate", 
			    G_CALLBACK (expand_or_collapse_all), GUINT_TO_POINTER (FALSE));
  for (mb_menu_items_cnt = 0; mb_menu_items_cnt < NUMBER_OF_VIEW_AND_OPTIONS; mb_menu_items_cnt++)
    g_signal_connect_swapped (mb_view_and_options[mb_menu_items_cnt], "activate", 
			      G_CALLBACK (change_view_and_options), GUINT_TO_POINTER (mb_menu_items_cnt));

  g_signal_connect (mb_about, "activate", G_CALLBACK (about), NULL);

  g_signal_connect (tb[TB_NEW], "clicked", G_CALLBACK (new_menu), NULL);
  g_signal_connect (tb[TB_OPEN], "clicked", G_CALLBACK (open_menu), NULL);
  g_signal_connect_swapped (tb[TB_SAVE], "clicked", G_CALLBACK (save_menu), NULL);
  g_signal_connect (tb[TB_SAVE_AS], "clicked", G_CALLBACK (save_menu_as), NULL);
  g_signal_connect_swapped (tb[TB_MOVE_UP], "clicked", G_CALLBACK (move_selection), GUINT_TO_POINTER (UP));
  g_signal_connect_swapped (tb[TB_MOVE_DOWN], "clicked", G_CALLBACK (move_selection), GUINT_TO_POINTER (DOWN));
  g_signal_connect_swapped (tb[TB_REMOVE], "clicked", G_CALLBACK (remove_rows), "toolbar");
  g_signal_connect (tb[TB_FIND], "clicked", G_CALLBACK (show_or_hide_find_grid), NULL);
  g_signal_connect_swapped (tb[TB_EXPAND_ALL], "clicked", G_CALLBACK (expand_or_collapse_all), GUINT_TO_POINTER (TRUE));
  g_signal_connect_swapped (tb[TB_COLLAPSE_ALL], "clicked",
			    G_CALLBACK (expand_or_collapse_all), GUINT_TO_POINTER (FALSE));
  g_signal_connect (tb[TB_QUIT], "clicked", G_CALLBACK (quit_program), NULL);

  for (buttons_cnt = 0; buttons_cnt < ACTION_OR_OPTION; buttons_cnt++)
    // Dyn. alloc. mem. is not freed here, since the texts have to be present as long as the program runs.
    g_signal_connect_swapped (bt_add[buttons_cnt], "clicked", G_CALLBACK (add_new), 
			      g_ascii_strdown (bt_add_txts[buttons_cnt], -1));
  /* The signal for the "Action/Option" button is always disconnected first, before it is reconnected with the 
     currently appropriate function. This means that the function and parameter are meaningless here since the signal 
     is an unused "dummy", but nevertheless necessary because there has to be a signal that can be disconnected before 
     adding a new one. */
  handler_id_action_option_button_clicked = g_signal_connect_swapped (bt_add[ACTION_OR_OPTION], "clicked", 
  								      G_CALLBACK (add_new), NULL);

  handler_id_action_option_combo_box = g_signal_connect (action_option, "changed", 
							 G_CALLBACK (show_action_options), NULL);
  g_signal_connect_swapped (action_option_done, "clicked", G_CALLBACK (action_option_insert), "by combo box");
  g_signal_connect (action_option_cancel, "clicked", G_CALLBACK (hide_action_option), NULL);
  handler_id_show_startupnotify_options = g_signal_connect (options_check_button, "toggled", 
							    G_CALLBACK (show_startupnotify_options), NULL);
  
  g_signal_connect (find_button_entry_row[CLOSE], "clicked", G_CALLBACK (show_or_hide_find_grid), NULL);
  for (buttons_cnt = BACK; buttons_cnt <= FORWARD; buttons_cnt++) 
    g_signal_connect_swapped (find_button_entry_row[buttons_cnt], "clicked", 
			      G_CALLBACK (jump_to_previous_or_next_occurrence),
			      GUINT_TO_POINTER ((buttons_cnt == FORWARD)));
  g_signal_connect (find_entry, "activate", G_CALLBACK (run_search), NULL);
  for (columns_cnt = 0; columns_cnt < NUMBER_OF_COLUMNS - 1; columns_cnt++) {
    handler_id_find_in_columns[columns_cnt] = g_signal_connect_swapped (find_in_columns[columns_cnt], "clicked", 
									G_CALLBACK (find_buttons_management), "");
  }
  g_signal_connect_swapped (find_in_all_columns, "clicked", G_CALLBACK (find_buttons_management), "ALL");
  g_signal_connect_swapped (find_match_case, "clicked", G_CALLBACK (find_buttons_management), NULL);
  g_signal_connect_swapped (find_regular_expression, "clicked", G_CALLBACK (find_buttons_management), NULL);
  
  for (entry_fields_cnt = 0; entry_fields_cnt < NUMBER_OF_ENTRY_FIELDS; entry_fields_cnt++)
    g_signal_connect (entry_fields[entry_fields_cnt], "activate", G_CALLBACK (change_row), NULL);
  g_signal_connect (icon_chooser, "clicked", G_CALLBACK (icon_choosing_by_button_or_context_menu), NULL);
  g_signal_connect (remove_icon, "clicked", G_CALLBACK (remove_icons_from_menus_or_items), NULL);

  g_signal_connect (options_command, "activate", G_CALLBACK (single_field_entry), NULL);
  g_signal_connect (options_prompt, "activate", G_CALLBACK (single_field_entry), NULL);
  for (snotify_opts_cnt = NAME; snotify_opts_cnt < NUMBER_OF_STARTUPNOTIFY_OPTS; snotify_opts_cnt++)
    g_signal_connect (suboptions_fields[snotify_opts_cnt], "activate", G_CALLBACK (single_field_entry), NULL);

  // Deactivate interactive search, since kickshaw provides its own search functionality.
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), -1);

  // Deactivate user defined column search by default.
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find_in_all_columns), TRUE);


  // --- Load settings and standard menu file, if existent. ---


  settings_file_path = g_strconcat (getenv ("HOME"), "/.kickshawrc", NULL);
  if (!g_file_test (settings_file_path, G_FILE_TEST_EXISTS))
    write_settings ();
  else {
    if (!g_key_file_load_from_file (settings_file, settings_file_path, 
				    G_KEY_FILE_KEEP_COMMENTS, &settings_file_error)) {
      gchar *errmsg_txt = g_strdup_printf ("<b>Could not open settings file</b>\n<tt>%s</tt>\n<b>for reading!\n\n"
					   "<span foreground='#8a1515'>%s</span></b>", 
					   settings_file_path, settings_file_error->message);

      show_errmsg (errmsg_txt);

      // Cleanup
      g_error_free (settings_file_error);
      g_free (errmsg_txt);
    }
    else {
      for (view_and_opts_cnt = 0; view_and_opts_cnt < NUMBER_OF_VIEW_AND_OPTIONS; view_and_opts_cnt++) {
	gtk_check_menu_item_set_active 
	  (GTK_CHECK_MENU_ITEM (mb_view_and_options[view_and_opts_cnt]), 
	   g_key_file_get_boolean (settings_file, (view_and_opts_cnt < SORT_EXECUTE_AND_STARTUPN_OPTIONS) ? 
				   "VIEW" : "OPTIONS", 
				   gtk_menu_item_get_label ((GtkMenuItem *) mb_view_and_options[view_and_opts_cnt]), 
				   NULL));
      }
      // Cleanup
      g_key_file_free (settings_file);
    }
  }
  // Cleanup
  g_free (settings_file_path);

  gtk_widget_show_all (window);
  // Defaults
  gtk_widget_hide (action_option_grid);
  gtk_widget_hide (find_grid);

 /* The height of the message label is set to be identical to the one of the buttons, so the button grid doesn't 
    shrink if the buttons are missing. This can only be done after all widgets have already been added to the grid, 
    because only then the height of the button grid (with buttons shown) can be determined with the correct value. */
  gtk_widget_set_size_request (bt_bar_label, -1, gtk_widget_get_allocated_height (bt_add[0]));

  // Load standard menu file, if existent.
  if (g_file_test (new_filename = g_strconcat (getenv ("HOME"), "/.config/openbox/menu.xml", NULL), G_FILE_TEST_EXISTS))
    get_tree_row_data (new_filename);
  else
    g_free (new_filename);

  // Sets settings for menu- and toolbar, that depend on whether there is a menu file or not.
  row_selected ();

  gtk_widget_grab_focus (treeview);
}

/* 

   Creates a label for an "Add new" button, adds it to a grid, which is added to the button.

*/

static void add_button_content (GtkWidget *button, 
                                gchar     *label_text)
{
  GtkWidget *grid = gtk_grid_new ();
  GtkWidget *label = gtk_label_new (label_text);

  gtk_container_set_border_width (GTK_CONTAINER (grid), 2);
  gtk_container_add (GTK_CONTAINER (grid), (*label_text) ? label : bt_add_action_option_label);
  gtk_container_add (GTK_CONTAINER (button), grid);
}

/* 

   Shows an error message dialog.

*/

void show_errmsg (gchar *errmsg_raw_txt)
{
  GtkWidget *dialog;
  gchar *label_txt =  g_strdup_printf ((!strstr (errmsg_raw_txt, "<b>")) ? "<b>%s</b>" : "%s", errmsg_raw_txt);
 
  create_dialog (&dialog, "Error", GTK_STOCK_DIALOG_ERROR, "Close", NULL, NULL, label_txt, TRUE);

  // Cleanup
  g_free (label_txt);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/* 

   Creates a new label with some formattings.

*/

GtkWidget *new_label_with_formattings (gchar *label_txt)
{
  return gtk_widget_new (GTK_TYPE_LABEL, "label", label_txt, "xalign", 0.0, "use-markup", TRUE, "wrap", TRUE, NULL);
}

/* 

   Convenience function for the creation of a dialog window.

*/

GtkWidget *create_dialog (GtkWidget **dialog, 
			  gchar      *dialog_title, 
			  gchar      *stock_id, 
			  gchar      *button_txt_1, 
			  gchar      *button_txt_2, 
			  gchar      *button_txt_3, 
			  gchar      *label_txt, 
			  gboolean    show_immediately)
{
  GtkWidget *content_area, *label;
  gchar *label_txt_with_addl_border = g_strdup_printf ("\n%s\n", label_txt);

  *dialog = gtk_dialog_new_with_buttons (dialog_title, GTK_WINDOW (window), GTK_DIALOG_MODAL, 
					 button_txt_1, 1, button_txt_2, 2, button_txt_3, 3,
					 NULL);
  
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (*dialog));
  gtk_orientable_set_orientation (GTK_ORIENTABLE (content_area), GTK_ORIENTATION_VERTICAL);
  gtk_container_set_border_width (GTK_CONTAINER (content_area), 10);
  gtk_container_add (GTK_CONTAINER (content_area), gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG));
  label = new_label_with_formattings (label_txt_with_addl_border);
  gtk_widget_set_size_request (label, 570, -1);
  gtk_container_add (GTK_CONTAINER (content_area), label);
  if (show_immediately)
    gtk_widget_show_all (*dialog);

  // Cleanup
  g_free (label_txt_with_addl_border);

  return content_area;
}

/* 

   Sets if the selection state of a node may be toggled.

*/

static gboolean selection_block_unblock (GtkTreeSelection G_GNUC_UNUSED *selection, 
					 GtkTreeModel     G_GNUC_UNUSED *model,
					 GtkTreePath      G_GNUC_UNUSED *path, 
					 gboolean         G_GNUC_UNUSED  path_currently_selected, 
					 gpointer                        block_state)
{
  return GPOINTER_TO_INT (block_state);
}

/* 

   A right click inside the treeview opens the context menu, a left click activates blocking of
   selection changes if more than one row has been selected and one of these rows has been clicked again.

*/

static gboolean mouse_pressed (GtkTreeView *treeview, GdkEventButton *event)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  if (event->type == GDK_BUTTON_PRESS) {
    if (event->button == 3) {
      create_context_menu (event);
      return TRUE;
    }
    else if (event->button == 1 && gtk_tree_selection_count_selected_rows (selection) > 1) {
      if (event->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK))
	return FALSE;

      GtkTreePath *path;

      if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview), event->x, event->y, &path, NULL, NULL, NULL))
	return FALSE;
      if (gtk_tree_selection_path_is_selected (selection, path))
	gtk_tree_selection_set_select_function (selection, (GtkTreeSelectionFunc) selection_block_unblock, 
						GINT_TO_POINTER (FALSE), NULL);

      // Cleanup
      gtk_tree_path_free (path);
    }
  }

  return FALSE;
}

/* 

   Unblocks selection changes.

*/

static gboolean mouse_released (void)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  if (gtk_tree_selection_count_selected_rows (selection) < 2)
    return FALSE;

  gtk_tree_selection_set_select_function (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)), 
					  (GtkTreeSelectionFunc) selection_block_unblock, GINT_TO_POINTER (TRUE), NULL);

  return FALSE;
}

/* 

   Sets an iterator for the toplevel of a given path.

*/

void get_toplevel_iter_from_path (GtkTreeIter *local_iter, GtkTreePath *local_path) {
  gchar *toplevel_str = g_strdup_printf ("%i", gtk_tree_path_get_indices (local_path)[0]);

  gtk_tree_model_get_iter_from_string (model, local_iter, toplevel_str);

  // Cleanup
  g_free (toplevel_str);
}

/* 

   Checks for existing options of an Execute action or startupnotify option.

*/

void check_for_existing_options (GtkTreeIter  *parent, 
				 guint8        number_of_opts, 
				 gchar       **options_array, 
				 gboolean     *opts_exist)
{
  GtkTreeIter iter_loop;
  gchar *menu_element_txt_loop;
  gint ch_cnt;
  guint8 opts_cnt;

  for (ch_cnt = 0; ch_cnt < gtk_tree_model_iter_n_children (model, parent); ch_cnt++) {
    gtk_tree_model_iter_nth_child (model, &iter_loop, parent, ch_cnt);
    gtk_tree_model_get (model, &iter_loop, TS_MENU_ELEMENT, &menu_element_txt_loop, -1);
    for (opts_cnt = 0; opts_cnt < number_of_opts; opts_cnt++)
      if (streq (menu_element_txt_loop, options_array[opts_cnt]))
	opts_exist[opts_cnt] = TRUE;

    // Cleanup
    g_free (menu_element_txt_loop);
  }
}

/* 

   Looks for invisible descendants of a row.

*/

gboolean check_if_invisible_descendant_exists (GtkTreeModel              *filter_model,
					       GtkTreePath G_GNUC_UNUSED *filter_path,
					       GtkTreeIter               *filter_iter,
					       gboolean                  *at_least_one_descendant_is_invisible)
{
  gchar *menu_element_txt_loop, *type_txt_loop;

  gtk_tree_model_get (filter_model, filter_iter, 
		      TS_MENU_ELEMENT, &menu_element_txt_loop, 
		      TS_TYPE, &type_txt_loop, 
		      -1);

  *at_least_one_descendant_is_invisible = (!menu_element_txt_loop && !streq (type_txt_loop, "separator"));

  // Cleanup
  g_free (menu_element_txt_loop);
  g_free (type_txt_loop);

  return *at_least_one_descendant_is_invisible;
}

/* 

   Looks for invisible ancestors of a given path and
   returns the value of the elements visiblity column for the first found one.

*/

gchar *check_if_invisible_ancestor_exists (GtkTreeModel *local_model, GtkTreePath *path)
{
  if (gtk_tree_path_get_depth (path) == 1)
    return NULL;

  GtkTreePath *path_copy = gtk_tree_path_copy (path);

  GtkTreeIter iter_loop;
  gchar *element_ancestor_visibility_txt_loop;

  do {
    gtk_tree_path_up (path_copy);
    gtk_tree_model_get_iter (local_model, &iter_loop, path_copy);
    gtk_tree_model_get (local_model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_ancestor_visibility_txt_loop, -1);
    if (element_ancestor_visibility_txt_loop && g_str_has_prefix (element_ancestor_visibility_txt_loop, "invisible")) {
      // Cleanup
      gtk_tree_path_free (path_copy);

      return element_ancestor_visibility_txt_loop;
    }

    // Cleanup
    g_free (element_ancestor_visibility_txt_loop);
  } while (gtk_tree_path_get_depth (path_copy) > 1);

  // Cleanup
  gtk_tree_path_free (path_copy);

  return NULL;
}

/* 

   Checks if there is a match for a search, used for replacing several matches at once.

*/

static gboolean evaluate_match (const GMatchInfo *match_info, 
				GString          *result, 
				gpointer          data)
{
  gchar *match = g_match_info_fetch (match_info, 0);
  gchar *single_result = g_hash_table_lookup ((GHashTable *) data, match);

  g_string_append (result, single_result);

  // Cleanup
  g_free (match);

  return FALSE;
}

/* 

   Sets attributes like foreground and background colour, visibility of cell renderers and 
   editability of cells according to certain conditions. Also highlights search results.

*/

static void set_column_attributes (GtkTreeViewColumn G_GNUC_UNUSED *cell_column, 
				   GtkCellRenderer                 *txt_renderer,
				   GtkTreeModel                    *cell_model, 
                                   GtkTreeIter                     *cell_iter, 
				   gpointer                         column_number_pointer)
{
  enum { CELL_DATA_MENU_ELEMENT_TXT, CELL_DATA_TYPE_TXT, CELL_DATA_VALUE_TXT, CELL_DATA_MENU_ID_TXT, 
	 CELL_DATA_EXECUTE_TXT, CELL_DATA_ELEMENT_VISIBILITY_TXT, NUMBER_OF_CELL_DATA_VARS };
  enum { NONE, INTEGRATED_INV, UNINTEGRATED_INV };

  guint column_number = GPOINTER_TO_UINT (column_number_pointer);
  GtkTreePath *cell_path = gtk_tree_model_get_path (cell_model, cell_iter);
  gchar *cell_data[NUMBER_OF_CELL_DATA_VARS];
  GdkPixbuf *cell_data_icon;
  guint cell_data_icon_img_status;

  gchar *element_visibility_ancestor_txt = NULL;
  guint8 unintegrated_or_integrated_inv = NONE;

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gboolean row_is_selected = gtk_tree_selection_iter_is_selected (selection, cell_iter);

  // Defaults
  gboolean visualise_txt_renderer = TRUE;
  gboolean visualise_bool_renderer = FALSE;

  gchar *background;
  gboolean background_set;
  gchar *highlighted_txt = NULL;

  gboolean show_icons = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[SHOW_ICONS]));
  gboolean show_separators_in_bold_type = 
    gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[SHOW_SEP_IN_BOLD_TYPE]));
  gboolean keep_highlighting =
    gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM 
				    (mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_KEEP_HIGHL]));

  gtk_tree_model_get (cell_model, cell_iter, 
		      TS_ICON_IMG, &cell_data_icon, 
		      TS_ICON_IMG_STATUS, &cell_data_icon_img_status,
		      TS_MENU_ELEMENT, &cell_data[CELL_DATA_MENU_ELEMENT_TXT], 
		      TS_TYPE, &cell_data[CELL_DATA_TYPE_TXT], 
		      TS_VALUE, &cell_data[CELL_DATA_VALUE_TXT], 
		      TS_MENU_ID, &cell_data[CELL_DATA_MENU_ID_TXT], 
		      TS_EXECUTE, &cell_data[CELL_DATA_EXECUTE_TXT], 
		      TS_ELEMENT_VISIBILITY, &cell_data[CELL_DATA_ELEMENT_VISIBILITY_TXT], 
		      -1);

  /* Set the cell renderer type of the "Value" column to toggle if it is a "prompt" option of a non-Execute action or 
     an "enabled" option of a "startupnotify" option block. */

  if (column_number == COL_VALUE && streq (cell_data[CELL_DATA_TYPE_TXT], "option") && 
      streq_any (cell_data[CELL_DATA_MENU_ELEMENT_TXT], "prompt", "enabled", NULL)) {
    GtkTreeIter parent;
    gchar *cell_data_menu_element_parent_txt;

    gtk_tree_model_iter_parent (cell_model, &parent, cell_iter);
    gtk_tree_model_get (model, &parent, TS_MENU_ELEMENT, &cell_data_menu_element_parent_txt, -1);
    if (!streq (cell_data_menu_element_parent_txt, "Execute")) {
      visualise_txt_renderer = FALSE;
      visualise_bool_renderer = TRUE;
      g_object_set (renderer[BOOL_RENDERER], "active", streq (cell_data[CELL_DATA_VALUE_TXT], "yes"), NULL);
    }
    // Cleanup
    g_free (cell_data_menu_element_parent_txt);
  }

  g_object_set (txt_renderer, "visible", visualise_txt_renderer, NULL);
  g_object_set (renderer[BOOL_RENDERER], "visible", visualise_bool_renderer, NULL);
  g_object_set (renderer[PIXBUF_RENDERER], "visible", (show_icons && cell_data_icon), NULL);
  g_object_set (renderer[EXCL_TXT_RENDERER], "visible", 
		(show_icons && cell_data_icon && cell_data_icon_img_status), NULL);

  /* If the icon is one of the two built-in types that indicate an invalid path or icon image, 
     set two red exclamation marks behind it to clearly distinguish this icon from icons of valid image files. */ 
  if (column_number == COL_MENU_ELEMENT && cell_data_icon_img_status)
    g_object_set (renderer[EXCL_TXT_RENDERER], "markup", "<span foreground='red'>!!</span>", NULL);

  // Emphasis that a menu, pipe menu or item has no label (=invisible).
  if (column_number == COL_MENU_ELEMENT && 
      streq_any (cell_data[CELL_DATA_TYPE_TXT], "menu", "pipe menu", "item", NULL) && 
      !cell_data[CELL_DATA_MENU_ELEMENT_TXT]) {
    g_object_set (txt_renderer, "text", "(No label)", NULL);
  }

  if ((cell_data[CELL_DATA_ELEMENT_VISIBILITY_TXT] && 
       !streq (cell_data[CELL_DATA_ELEMENT_VISIBILITY_TXT], "visible")) || 
      (element_visibility_ancestor_txt = check_if_invisible_ancestor_exists (cell_model, cell_path))) {
    unintegrated_or_integrated_inv = 
      (g_str_has_suffix ((element_visibility_ancestor_txt) ? element_visibility_ancestor_txt : 
			 cell_data[CELL_DATA_ELEMENT_VISIBILITY_TXT], "unintegrated menu")) ? 
      UNINTEGRATED_INV : INTEGRATED_INV;
  }
  background = (unintegrated_or_integrated_inv == INTEGRATED_INV) ? "#656772" : "#364074";
  background_set = (unintegrated_or_integrated_inv && keep_highlighting);

  // If a search is going on, highlight all matches.
  if (column_number < COL_ELEMENT_VISIBILITY && !gtk_widget_get_visible (action_option_grid) && *search_term_str && 
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_in_columns[column_number])) &&
      check_for_match (search_term_str, cell_iter, column_number)) {
    gchar *search_term_str_escaped = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_regular_expression))) ? 
      NULL : g_regex_escape_string (search_term_str, -1);
    GRegex *regex = g_regex_new ((search_term_str_escaped) ? search_term_str_escaped : search_term_str, 
				 (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find_match_case))) ? 
				 0 : G_REGEX_CASELESS, 0, NULL);
    GMatchInfo *match_info = NULL;
    gchar *match;
    GSList *matches = NULL;
    GSList *replacement_txts = NULL;
    GHashTable *hash_table = g_hash_table_new (g_str_hash, g_str_equal);
    gchar *highlighted_txt_core;

    g_regex_match (regex, cell_data[column_number], 0, &match_info);
    while (g_match_info_matches (match_info)) {
      if (!g_slist_find_custom (matches, match = g_match_info_fetch (match_info, 0), (GCompareFunc) strcmp)) {
	matches = g_slist_prepend (matches, g_strdup (match));
	replacement_txts = g_slist_prepend (replacement_txts, 
					    g_strconcat ("<span background='", (row_is_selected) ? "black'>" : 
							 "yellow' foreground='black'>", match, "</span>", NULL));
	g_hash_table_insert (hash_table, matches->data, replacement_txts->data);
      }
      g_match_info_next (match_info, NULL);

      // Cleanup
      g_free (match);
    }

    // Cleanup
    g_match_info_free (match_info);

    // Replace all findings with a highlighted version at once.
    highlighted_txt_core = g_regex_replace_eval (regex, cell_data[column_number], -1, 0, 0, 
						 evaluate_match, hash_table, NULL);
    highlighted_txt = (!background_set) ? g_strdup (highlighted_txt_core) : 
      g_strdup_printf ("<span foreground='white'>%s</span>", highlighted_txt_core);

    g_object_set (txt_renderer, "markup", highlighted_txt, NULL);

    // Cleanup
    g_free (search_term_str_escaped);
    g_slist_free_full (matches, (GDestroyNotify) g_free);
    g_slist_free_full (replacement_txts, (GDestroyNotify) g_free);
    g_hash_table_destroy (hash_table);
    g_regex_unref (regex);
    g_free (highlighted_txt_core);
  }

  // Set forward and background font and cell colours. Also set editability of cells.
  g_object_set (txt_renderer, "weight", 
		(show_separators_in_bold_type && streq (cell_data[CELL_DATA_TYPE_TXT], "separator")) ? 1000 : 400, 
		"family", (streq (cell_data[CELL_DATA_TYPE_TXT], "separator")) ? 
		"monospace, courier new, courier" : "sans, sans-serif, arial, helvetica", 
		"foreground", "white", "foreground-set", (row_is_selected || (background_set && !highlighted_txt)),
		"background", background, "background-set", background_set, "editable", 
		(((column_number == COL_MENU_ELEMENT && 
		   (streq (cell_data[CELL_DATA_TYPE_TXT], "separator") || 
		    (cell_data[CELL_DATA_MENU_ELEMENT_TXT] && 
		     streq_any (cell_data[CELL_DATA_TYPE_TXT], "menu", "pipe menu", "item", NULL)))) || 
		  (column_number == COL_VALUE && streq (cell_data[CELL_DATA_TYPE_TXT], "option")) || 
		  (column_number == COL_MENU_ID && 
		   streq_any (cell_data[CELL_DATA_TYPE_TXT], "menu", "pipe menu", NULL)) ||
		  (column_number == COL_EXECUTE && streq (cell_data[CELL_DATA_TYPE_TXT], "pipe menu")))), 
		"editable-set", FALSE, NULL);

  for (guint8 renderer_cnt = EXCL_TXT_RENDERER; renderer_cnt < NUMBER_OF_RENDERERS; renderer_cnt++)
    g_object_set (renderer[renderer_cnt], "cell-background", background, "cell-background-set", background_set, NULL);

  if (highlighted_txt && gtk_cell_renderer_get_visible (renderer[BOOL_RENDERER]))
    g_object_set (renderer[BOOL_RENDERER], "cell-background", "yellow", "cell-background-set", TRUE, NULL);

  // Cleanup
  g_free (element_visibility_ancestor_txt);
  g_free (highlighted_txt);
  gtk_tree_path_free (cell_path);
  free_elements_of_static_string_array (cell_data, NUMBER_OF_CELL_DATA_VARS, FALSE);
  unref_icon (&cell_data_icon, FALSE);
}

/* 

   Changes certain aspects of the tree view or misc. settings.

*/

void change_view_and_options (gpointer activated_menu_item_pointer)
{
  guint8 activated_menu_item = GPOINTER_TO_UINT (activated_menu_item_pointer);
  gboolean menu_item_activated = 
    (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[activated_menu_item])));

  if (activated_menu_item <= SHOW_ELEMENT_VISIBILITY_COL_ACT) {
    /* SHOW_MENU_ID_COL (0)                + 3 = COL_MENU_ID (3)
       SHOW_EXECUTE_COL (1)                + 3 = COL_EXECUTE (4)
       SHOW_ELEMENT_VISIBILITY_COL_ACT (2) + 3 = COL_ELEMENT_VISIBILITY (5) */
    guint8 column_position = activated_menu_item + 3;

    gtk_tree_view_column_set_visible (columns[column_position], menu_item_activated);

    if (column_position == COL_ELEMENT_VISIBILITY) {
      if (!menu_item_activated) {
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM 
					(mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_KEEP_HIGHL]), TRUE);
      }
      gtk_widget_set_sensitive (mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_KEEP_HIGHL], menu_item_activated);
      gtk_widget_set_sensitive (mb_view_and_options[SHOW_ELEMENT_VISIBILITY_COL_DONT_KEEP_HIGHL], menu_item_activated);
    }
  }
  else if (activated_menu_item == DRAW_ROWS_IN_ALT_COLOURS)
    g_object_set (treeview, "rules-hint", menu_item_activated, NULL);
  else if (activated_menu_item >= NO_GRID_LINES && activated_menu_item <= BOTH) {
    guint8 grid_settings_cnt, grid_lines_type_cnt;

    for (grid_settings_cnt = NO_GRID_LINES, grid_lines_type_cnt = GTK_TREE_VIEW_GRID_LINES_NONE; 
	 grid_settings_cnt <= BOTH; 
	 grid_settings_cnt++, grid_lines_type_cnt++) {
      if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[grid_settings_cnt]))) {
	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), grid_lines_type_cnt);
	break;
      }
    }
  }
  else if (activated_menu_item == SHOW_TREE_LINES)
    gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW (treeview), menu_item_activated);
  else if (activated_menu_item == SORT_EXECUTE_AND_STARTUPN_OPTIONS) {
    if ((autosort_options = menu_item_activated))
      gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) sort_loop_after_sorting_activation, NULL);
    /* If autosorting has been activated, a selected option might have changed its position to the bottom or top, if so, 
       the move arrows of the toolbar have to be readjusted. If it has been deactivated, it is now possible to move a 
       selected option again, so a readjustment is also necessary. */
    row_selected ();
  }
  else
    gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview)); // If icon visibility has been switched on.

  write_settings ();
}

/* 

   Clears the data that is held throughout the running time of the program.

*/

void clear_global_static_data (void)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  free_and_reassign (filename, NULL);
  g_slist_free_full (menu_ids, (GDestroyNotify) g_free);
  menu_ids = NULL;
  if (rows_with_icons)
    stop_timer ();
  if (gtk_widget_get_visible (find_grid))
    show_or_hide_find_grid ();
  g_signal_handler_block (selection, handler_id_row_selected);
  gtk_tree_store_clear (treestore);
  g_signal_handler_unblock (selection, handler_id_row_selected);
  change_done = FALSE;
}

/* 

   Expands the tree view so the whole structure is visible or
   collapses the tree view so just the toplevel elements are visible.

*/

static void expand_or_collapse_all (gpointer expand_pointer)
{
  gboolean expand = GPOINTER_TO_UINT (expand_pointer);

  if (expand)
    gtk_tree_view_expand_all (GTK_TREE_VIEW (treeview));
  else {
    gtk_tree_view_collapse_all (GTK_TREE_VIEW (treeview));
    gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview));
  }

  gtk_widget_set_sensitive (mb_expand_all_nodes, !expand);
  gtk_widget_set_sensitive (mb_collapse_all_nodes, expand);
  gtk_widget_set_sensitive ((GtkWidget *) tb[TB_EXPAND_ALL], !expand);
  gtk_widget_set_sensitive ((GtkWidget *) tb[TB_COLLAPSE_ALL], expand);
}

/* 

   Dialog window with information about author, version, license, website etc.

*/

static void about (void) 
{
  GtkWidget *dialog = gtk_about_dialog_new ();
  const gchar *authors[] = { "Marcus Schaetzle", NULL };

  gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (dialog), "Kickshaw");
  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dialog), "0.5 RC2");
  gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (dialog), "(c) 2010-2013 Marcus Schaetzle");
  gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (dialog), "\
Copyright (c) 2010-2013        Marcus Schaetzle\n\n\
\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or\n\
(at your option) any later version.\n\n\
\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n\
GNU General Public License for more details.\n\n\
\
You should have received a copy of the GNU General Public License along\n\
with Kickshaw. If not, see http://www.gnu.org/licenses/ .");
  gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (dialog), "https://savannah.nongnu.org/projects/obladi/");
  gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (dialog), authors);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/* 

   Asks to continue if there are unsaved changes.

*/

gboolean unsaved_changes (void)
{
  GtkWidget *dialog;
  gchar *dialog_txt = "<b>This menu has unsaved changes. Continue anyway?</b>"; 

  gint result;

  enum { QUIT_DISPITE_UNSAVED_CHANGES = 1, DO_NOT_QUIT };

  create_dialog (&dialog, "Menu has unsaved changes", GTK_STOCK_DIALOG_WARNING, 
		 GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL, dialog_txt, TRUE);

  result = gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return (result == QUIT_DISPITE_UNSAVED_CHANGES);
}

/* 

   Clears treestore and global variables.

*/

void new_menu (void)
{
  if (change_done && !unsaved_changes ())
    return;

  gtk_window_set_title (GTK_WINDOW (window), "Kickshaw");

  clear_global_static_data ();

  gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview));
  row_selected (); // Switches the settings for menu- and toolbar to that of an empty menu.
}

/* 

   Quits program, if there are unsaved changes a confirmation dialog window is shown.

*/

static void quit_program (void) {
  if (change_done && !unsaved_changes ())
    return;

  exit (EXIT_SUCCESS);
}

/* 

   Stores the date of the last modification of an icon image file.

*/

gchar *get_modified_date_for_icon (gchar *icon_path)
{
  GFile *file = g_file_new_for_path (icon_path);
  GFileInfo *file_info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED, 
					    G_FILE_QUERY_INFO_NONE, NULL, NULL);
  GTimeVal icon_modified;

  g_file_info_get_modification_time (file_info, &icon_modified);
  
  // Cleanup
  g_object_unref (file);
  g_object_unref (file_info);

  return g_time_val_to_iso8601 (&icon_modified);
}

/* 

   Adds a row that contains an icon to a list.

*/

static gboolean add_icon_occurrence_to_list (GtkTreeModel G_GNUC_UNUSED *local_model, 
					     GtkTreePath                *local_path, 
                                             GtkTreeIter                *local_iter)
{
  GdkPixbuf *icon;

  gtk_tree_model_get (model, local_iter, TS_ICON_IMG, &icon, -1);
  if (icon) {
    rows_with_icons = g_slist_prepend (rows_with_icons, gtk_tree_row_reference_new (model, local_path));

    // Cleanup
    unref_icon (&icon, FALSE);
  }

  return FALSE;
}

/* 

   Creates a list that contains all rows with an icon.

*/

void create_list_of_icon_occurrences (void)
{
  gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) add_icon_occurrence_to_list, NULL);
  if (rows_with_icons)
    g_timeout_add (1000, (GSourceFunc) check_for_external_file_and_settings_changes, "timer");
}

/* 

   Activates "Save" menubar item/toolbar button (provided that there is a filename) if a change has been done.
   Also sets a global veriable so a program-wide check for a change is possible.
   If a search is still active, the list of results is recreated.     
   A list of rows with icons is (re)created, too.

*/

void activate_change_done (void)
{
  if (filename) {
    gtk_widget_set_sensitive (mb_file_menu_items[MB_SAVE], TRUE);
    gtk_widget_set_sensitive ((GtkWidget *) tb[TB_SAVE], TRUE);
  }
  
  if (gtk_widget_get_visible (find_grid))
    create_list_of_rows_with_found_occurrences ();

  if (rows_with_icons)
    stop_timer ();
  create_list_of_icon_occurrences ();

  change_done = TRUE;
}

/* 

   Writes all view and option settings into a file.

*/

void write_settings (void)
{
  FILE *settings_file;
  gchar *settings_file_path = g_strconcat (getenv ("HOME"), "/.kickshawrc", NULL);
  
  if (!(settings_file = fopen (settings_file_path, "w"))) {
    gchar *errmsg_txt = g_strdup_printf ("<b>Could not open settings file</b>\n<tt>%s</tt>\n<b>for writing!</b>", 
					 settings_file_path);

    show_errmsg (errmsg_txt);

    // Cleanup
    g_free (errmsg_txt);
    g_free (settings_file_path);

    return;
  }

  // Cleanup
  g_free (settings_file_path);

  fputs ("[VIEW]\n\n", settings_file);

  for (guint8 view_and_opts_cnt = 0; view_and_opts_cnt < NUMBER_OF_VIEW_AND_OPTIONS; view_and_opts_cnt++) {
    if (view_and_opts_cnt == SORT_EXECUTE_AND_STARTUPN_OPTIONS)
      fputs ("\n[OPTIONS]\n\n", settings_file);
    fprintf (settings_file, "%s=%s\n", gtk_menu_item_get_label ((GtkMenuItem *) mb_view_and_options[view_and_opts_cnt]), 
	     (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[view_and_opts_cnt]))) ? 
	     "true" : "false");
  }

  fclose (settings_file);
}

/* 

   Creates icon images for the case of a broken path or an invalid image file.

*/

void create_invalid_icon_imgs (void)
{
  GdkPixbuf *invalid_icon_img_dialog_size;
  const gchar *stock_image;
  
  for (guint8 invalid_icon_img_cnt = 0; invalid_icon_img_cnt < NUMBER_OF_INVALID_ICON_IMGS; invalid_icon_img_cnt++) {
    unref_icon (&invalid_icon_imgs[invalid_icon_img_cnt], FALSE);

    stock_image = (invalid_icon_img_cnt == INVALID_PATH_ICON) ? GTK_STOCK_DIALOG_QUESTION : GTK_STOCK_MISSING_IMAGE;
    invalid_icon_img_dialog_size = 
      gtk_widget_render_icon_pixbuf (gtk_image_new_from_stock (stock_image, GTK_ICON_SIZE_DIALOG),
				     stock_image, GTK_ICON_SIZE_DIALOG);
    invalid_icon_imgs[invalid_icon_img_cnt] = 
      gdk_pixbuf_scale_simple (invalid_icon_img_dialog_size, font_size + 10, font_size + 10, GDK_INTERP_BILINEAR);

    // Cleanup
    g_object_unref (invalid_icon_img_dialog_size);
  }
}
