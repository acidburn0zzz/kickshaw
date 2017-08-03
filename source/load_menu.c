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

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "general_header_files/enum__invalid_icon_imgs_status.h"
#include "general_header_files/enum__ts_elements.h"
#include "general_header_files/enum__txt_fields.h"
#include "general_header_files/enum__view_and_options_menu_items.h"
#include "load_menu.h"

enum { TS_BUILD_ICON_IMG, TS_BUILD_ICON_IMG_STATUS, TS_BUILD_ICON_MODIFIED, TS_BUILD_ICON_PATH, TS_BUILD_MENU_ELEMENT, 
       TS_BUILD_TYPE, TS_BUILD_VALUE, TS_BUILD_MENU_ID, TS_BUILD_EXECUTE, TS_BUILD_ELEMENT_VISIBILITY, 
       TS_BUILD_PATH_DEPTH, NUMBER_OF_TS_BUILD_FIELDS };
enum { UNDEFINED, IGNORE_THIS_ERROR, IGNORE_ALL_UPCOMING_ERRORS };
enum { MENUS, ROOT_MENU, NUMBER_OF_MENU_LEVELS };
enum { MENUS_LIST, ITEMS_LIST, NUMBER_OF_MISSING_LABEL_LISTS };
// Enumerations for dialogs.
enum { YES = 1, NO };
enum { CHOOSE_FILE = 1, CHECK_LATER, IGNORE_ALL_ERRORS_AND_CHECK_LATER };
enum { KEEP_STATUS = 1, VISUALISE, DELETE };
enum { UNINTEGRATED_MENUS, MISSING_LABELS };

struct menu_building_data {
  gint line_nr;

  GSList *treestore_build[NUMBER_OF_TS_BUILD_FIELDS];
  guint current_path_depth;
  guint previous_path_depth;
  guint max_path_depth;

  GSList *menu_ids;
  GSList *toplevel_menu_ids[NUMBER_OF_MENU_LEVELS];

  guint8 icon_creation_error_handling;

  gchar *current_action;
  gchar *previous_type;

  gboolean dep_exe_cmds_have_been_converted;

  guint8 loading_stage;
  gboolean root_menu_finished;
};

static void start_element (GMarkupParseContext *parse_context, const gchar *element_name, const gchar **attribute_names,
			   const gchar **attribute_values, gpointer menu_building_pnt, GError **error);
static void end_element (GMarkupParseContext G_GNUC_UNUSED *parse_context, const gchar G_GNUC_UNUSED *element_name,
			 gpointer menu_building_pnt, GError G_GNUC_UNUSED **error);
static void element_text (GMarkupParseContext G_GNUC_UNUSED *parse_context, const gchar *text, 
			  gsize G_GNUC_UNUSED text_len, gpointer menu_building_pnt, GError G_GNUC_UNUSED **error);
static gboolean elements_visibility (GtkTreeModel *local_model, GtkTreePath *local_path,
				     GtkTreeIter *local_iter, GSList **menu_and_items_without_label);
static void create_dialogs_for_invisible_menus_and_items (guint8 dialog_type, GtkTreeSelection *selection, 
							  GSList **menus_and_items_without_label);
void get_tree_row_data (gchar *new_filename);
void open_menu (void);

/* 

   Parses start elements, including error checking.

*/

static void start_element (GMarkupParseContext  *parse_context, 
			   const gchar          *element_name,
			   const gchar         **attribute_names,
			   const gchar         **attribute_values,
			   gpointer              menu_building_pnt,
			   GError              **error) {
  struct menu_building_data *menu_building = (struct menu_building_data *) menu_building_pnt;
  
   // XML after the root-menu is discarded by Openbox.
  if (menu_building->root_menu_finished || streq (element_name, "openbox_menu"))
    return;

  // Unused since there is no need to differentiate, but needed by GLib's error reporting.
  GQuark domain = g_quark_from_string ("");

  const gchar *current_attribute_name;
  const gchar *current_attribute_value;
  guint number_of_attributes = g_strv_length ((gchar **) attribute_names);
  GSList **treestore_build;
  GSList **menu_ids = &(menu_building->menu_ids);
  guint current_path_depth = menu_building->current_path_depth;
  gchar *current_action = menu_building->current_action;
  gint line_nr = menu_building->line_nr;
  guint8 *loading_stage = &(menu_building->loading_stage);
  GSList **toplevel_menu_ids = menu_building->toplevel_menu_ids;

  gchar *valid;
  gboolean menu_id_found = FALSE; // Default

  // This is a local variant of txt_fields for constant values.
  G_GNUC_EXTENSION const gchar *txt_fields[] = { [0 ... NUMBER_OF_TXT_FIELDS] = NULL }; // Defaults

  const gchar *menu_id;
  const gchar *label = NULL;

  // Defaults = no icon
  GdkPixbuf *icon_img = NULL;
  guint8 icon_img_status = NONE_OR_NORMAL;
  gchar *icon_modified = NULL;
  gchar *icon_path = NULL;

  guint8 attribute_cnt, attribute_cnt2, ts_build_cnt;


  // --- Error checking ---


  // Too many attributes
  
  if ((streq (element_name, "menu") && number_of_attributes > 4) || 
      (streq (element_name, "item") && number_of_attributes > 2) || 
      (streq_any (element_name, "separator", "action", NULL) && number_of_attributes > 1)) {
    g_set_error (error, domain, line_nr, "Too many attributes for element '%s'", element_name);
    return;
  }

  // Invalid attributes

  for (attribute_cnt = 0; attribute_cnt < number_of_attributes; attribute_cnt++) {
    current_attribute_name = attribute_names[attribute_cnt];
    current_attribute_value = attribute_values[attribute_cnt];

    if ((streq (element_name, "menu") && !streq_any (current_attribute_name, "id", "label", "icon", "execute", NULL)) || 
	(streq (element_name, "item") && !streq_any (current_attribute_name, "label", "icon", NULL)) || 
	(streq (element_name, "separator") && !streq (current_attribute_name, "label")) || 
	(streq (element_name, "action") && !streq (current_attribute_name, "name"))) {
      if (streq (element_name, "menu"))
	valid = "are 'id', 'label', 'icon' and 'execute'";
      else if (streq (element_name, "item"))
	valid = "are 'label' and 'icon'";
      else if (streq (element_name, "separator"))
	valid = "is 'label'";
      else // action
	valid = "is 'name'";
      g_set_error (error, domain, line_nr, "Element '%s' has an invalid attribute '%s', valid %s", 
		   element_name, current_attribute_name, valid);
      return;
    }

    // Check if a menu has an ID. If it is defined as the root menu, check for a correct position.

    if (streq (element_name, "menu")) {
      if (streq (current_attribute_name, "id")) {
	menu_id_found = TRUE;
	menu_id = current_attribute_value;

	if (streq (current_attribute_value, "root-menu")) {
	  if (current_path_depth > 1) {
	    g_set_error (error, domain, line_nr, "%s", 
			 "The root menu can't be defined as a subelement of another menu element");
	    return;
	  }

	  *loading_stage = ROOT_MENU;
	}
      }
      else if (streq (current_attribute_name, "label"))
	label = current_attribute_value;
    }

    // Duplicate element attributes

    for (attribute_cnt2 = 0; attribute_cnt2 < number_of_attributes; attribute_cnt2++) {
      if (attribute_cnt == attribute_cnt2)
	continue;
      if (streq (current_attribute_name, attribute_names[attribute_cnt2])) {
	g_set_error (error, domain, line_nr, "Element '%s' has more than one '%s' attribute", 
		     element_name, current_attribute_name);
	return;
      }
    }
  }

  // Missing or duplicate menu IDs

  if (streq (element_name, "menu")) {
    if (!menu_id_found) {
      g_set_error (error, domain, line_nr, "Menu%s%s%s has no 'id' attribute", 
		   (label) ? " '" : "",  (label) ? label : "", (label) ? "'" : "");
      return;
    }

    if (g_slist_find_custom (*menu_ids, menu_id, (GCompareFunc) strcmp) &&
	!(*loading_stage == ROOT_MENU && current_path_depth == 2 && 
	  g_slist_find_custom (toplevel_menu_ids[MENUS], menu_id, (GCompareFunc) strcmp) && 
	  !g_slist_find_custom (toplevel_menu_ids[ROOT_MENU], menu_id, (GCompareFunc) strcmp))) {
      g_set_error (error, domain, line_nr, "'%s' is a menu ID that has already been defined before", menu_id);
      return;
    }
  }

  // Check if current element is a valid child of another element.

  if (current_path_depth > 1) {
    const GSList *element_stack = g_markup_parse_context_get_element_stack (parse_context);
    const gchar *parent_element_name = (g_slist_next (element_stack))->data;
    gchar *error_txt = NULL;

    if (streq (element_name, "menu") && !streq (parent_element_name, "menu"))
      error_txt = "A menu can only have another menu";
    else if (streq (element_name, "item") && !streq (parent_element_name, "menu"))
      error_txt = "An item can only have a menu";
    else if (streq (element_name, "separator") && !streq (parent_element_name, "menu"))
      error_txt = "A separator can only have a menu";
    else if (streq (element_name, "action") && !streq (parent_element_name, "item"))
      error_txt = "An action can only have an item";
    else if (streq (element_name, "prompt") && !(streq (parent_element_name, "action") && 
						 streq_any (current_action, "Execute", "Exit", "SessionLogout", NULL))) {
      error_txt = "A 'prompt' option can only have an 'Execute', 'Exit' or 'SessionLogout' action";
    }
    else if (streq (element_name, "command") && !(streq (parent_element_name, "action") && 
						  streq_any (current_action, "Execute", "Restart", NULL))) {
      error_txt = "A 'command' option can only have an 'Execute' or 'Restart' action";
    }
    else if (streq (element_name, "startupnotify") && !(streq (parent_element_name, "action") && 
							streq (current_action, "Execute"))) {
      error_txt = "A 'startupnotify' option can only have an 'Execute' action";
    }
    else if (streq_any (element_name, "enabled", "icon", "name", "wmclass", NULL) && 
	     !streq (parent_element_name, "startupnotify")) {
      g_set_error (error, domain, line_nr, "A%s '%s' option can only have a 'startupnotify' option as a parent", 
		   (streq_any (element_name, "enabled", "icon", NULL)) ? "n" : "", element_name);
      return;
    }

    else if (streq (menu_building->previous_type, "pipe menu") && 
	     menu_building->previous_path_depth < current_path_depth) {
      error_txt = "A pipe menu is a self-closing element, it can't be used";
    }  

    if (error_txt) {
      g_set_error (error, domain, line_nr, "%s as a parent", error_txt);
      return;
    }
  }


  // --- Retrieve attribute values


  if (streq_any (element_name, "menu", "item", "separator", NULL)) {
    txt_fields[TYPE_TXT] = element_name;

    for (attribute_cnt = 0; attribute_cnt < number_of_attributes; attribute_cnt++) {
      current_attribute_name = attribute_names[attribute_cnt];
      current_attribute_value = attribute_values[attribute_cnt];

      if (streq (current_attribute_name, "label"))
	txt_fields[MENU_ELEMENT_TXT] = current_attribute_value;
      if (!streq (element_name, "separator")) {
	if (streq (current_attribute_name, "icon"))
	  icon_path = g_strdup (current_attribute_value);
	if (streq (element_name, "menu")) {
	  if (streq (current_attribute_name, "id")) {
	    /* Root menu IDs are only included inside the menu_ids list 
	       if they did not already appear in an extern menu definition. */
	    if (!(*loading_stage == ROOT_MENU && 
		  (streq (current_attribute_value, "root-menu") || 
		   g_slist_find_custom (toplevel_menu_ids[MENUS], current_attribute_value, (GCompareFunc) strcmp))))
	      *menu_ids = g_slist_prepend (*menu_ids, g_strdup (current_attribute_value));
	  
	    if ((*loading_stage == MENUS && current_path_depth == 1) || 
		(*loading_stage == ROOT_MENU && current_path_depth == 2)) { // This excludes the "root-menu" id.
	      GSList **menu_ids_list = (*loading_stage == MENUS) ? 
		&toplevel_menu_ids[MENUS] : &toplevel_menu_ids[ROOT_MENU];

	      *menu_ids_list = g_slist_prepend (*menu_ids_list, g_strdup (current_attribute_value));
	    }
 
	    txt_fields[MENU_ID_TXT] = current_attribute_value;
	  }
	  else if (streq (current_attribute_name, "execute")) {
	    txt_fields[TYPE_TXT] = "pipe menu"; // Overwrites "menu".
	    txt_fields[EXECUTE_TXT] = current_attribute_value;
	  }
	}
      }
    }

    // Create icon images.

    if (icon_path) {
      GError *icon_creation_error = NULL;
      GdkPixbuf *icon_in_original_size;
  
      GtkWidget *dialog;

      guint8 *icon_creation_error_handling = &(menu_building->icon_creation_error_handling);

      gchar *manually_chosen_icon_path;

      gint result;

      if (!(icon_in_original_size = gdk_pixbuf_new_from_file (icon_path, &icon_creation_error))) {
	gboolean file_exists = g_file_test (icon_path, G_FILE_TEST_EXISTS);
	icon_img = gdk_pixbuf_copy (invalid_icon_imgs[(file_exists)]); // INVALID_FILE_ICON (TRUE) or INVALID_PATH_ICON
	icon_img_status = (file_exists) ? INVALID_FILE : INVALID_PATH;
	if (file_exists)
	  icon_modified = get_modified_date_for_icon (icon_path);

	if (*icon_creation_error_handling == IGNORE_ALL_UPCOMING_ERRORS)
	  g_error_free (icon_creation_error);
	else {
	  while (icon_creation_error) {
	    GString *dialog_txt = g_string_new ("");
	    g_string_append_printf (dialog_txt, "<b>Line %i:\nThe following error occurred " 
				    "while trying to create an icon for %s %s with ", 
				    line_nr, (txt_fields[MENU_ELEMENT_TXT]) ? "the" : 
				    ((streq (txt_fields[TYPE_TXT], "menu")) ? "a" : "an"), txt_fields[TYPE_TXT]);
	    if (streq_any (txt_fields[TYPE_TXT], "menu", "pipe menu", NULL))
	      g_string_append_printf (dialog_txt, "the menu ID '%s'", txt_fields[MENU_ID_TXT]);
	    else if (txt_fields[MENU_ELEMENT_TXT])
	      g_string_append_printf (dialog_txt, "the label '%s'", txt_fields[MENU_ELEMENT_TXT]);
	    else
	      g_string_append (dialog_txt, "no assigned label");
	    g_string_append_printf (dialog_txt, " from %s:\n\n<span foreground='#8a1515'>%s</span></b>\n\n"
				    "If you don't want to choose the correct/another file now, "
				    "you may ignore this or all following icon creation error messages now and "
				    "check later from inside the program. In this case all nodes that contain "
				    "menus and items with invalid icon paths will be shown expanded after the "
				    "loading process.", icon_path, icon_creation_error->message);

	    create_dialog (&dialog, "Icon creation error", GTK_STOCK_DIALOG_ERROR, 
			   "Choose file", "Check later", "Ignore all errors and check later", dialog_txt->str, TRUE);
	    
	    // Cleanup
	    g_string_free (dialog_txt, TRUE);

	    result = gtk_dialog_run (GTK_DIALOG (dialog));
	    gtk_widget_destroy (dialog);
	    switch (result) {
	    case CHOOSE_FILE:
	      if ((manually_chosen_icon_path = choose_icon ())) {
		// Cleanup and reset
		g_error_free (icon_creation_error);
		icon_creation_error = NULL;

		if ((icon_in_original_size = gdk_pixbuf_new_from_file (manually_chosen_icon_path, 
								       &icon_creation_error))) {
		  free_and_reassign (icon_path, g_strdup (manually_chosen_icon_path));
		  g_object_unref (icon_img);
		  icon_img_status = NONE_OR_NORMAL;
		}
	      }

	      // Cleanup
	      g_free (manually_chosen_icon_path);
	      break;
	    case CHECK_LATER:
	    case IGNORE_ALL_ERRORS_AND_CHECK_LATER:
	      *icon_creation_error_handling = (result == CHECK_LATER) ? IGNORE_THIS_ERROR : IGNORE_ALL_UPCOMING_ERRORS;
	      // Cleanup and reset
	      g_error_free (icon_creation_error);
	      icon_creation_error = NULL;
	      break;
	    }
	  }
	}
      }

      if (!icon_img_status) {
	icon_img = gdk_pixbuf_scale_simple (icon_in_original_size, font_size + 10, font_size + 10, GDK_INTERP_BILINEAR);
	free_and_reassign (icon_modified, get_modified_date_for_icon (icon_path));

	// Cleanup
	g_object_unref (icon_in_original_size);
      }

    }
  }
  else if (streq (element_name, "action")) {
    txt_fields[TYPE_TXT] = element_name;
    txt_fields[MENU_ELEMENT_TXT] = attribute_values[0]; // There is only one attribute.
    free_and_reassign (menu_building->current_action, g_strdup (attribute_values[0]));
  }

  if (g_regex_match_simple ("prompt|command|execute|startupnotify|enabled|wmclass|name|icon", 
			    element_name, G_REGEX_ANCHORED, 0)) {
    txt_fields[TYPE_TXT] = (!streq (element_name, "startupnotify")) ? "option" : "option block";
    txt_fields[MENU_ELEMENT_TXT] = element_name;
    if (streq (txt_fields[MENU_ELEMENT_TXT], "execute")) {
      txt_fields[MENU_ELEMENT_TXT] = "command";
      menu_building->dep_exe_cmds_have_been_converted = TRUE;
    }
  }


  // --- Store all values that are needed later to create a treeview row. ---


  for (ts_build_cnt = 0; ts_build_cnt < NUMBER_OF_TS_BUILD_FIELDS; ts_build_cnt++) {
    treestore_build = (GSList **) &(menu_building->treestore_build[ts_build_cnt]);
    switch (ts_build_cnt) {
    case TS_BUILD_ICON_IMG:
    case TS_BUILD_ICON_MODIFIED:
    case TS_BUILD_ICON_PATH:
      *treestore_build = g_slist_prepend (*treestore_build, (ts_build_cnt == TS_BUILD_ICON_IMG) ? (gpointer) icon_img : 
					  ((ts_build_cnt == TS_BUILD_ICON_MODIFIED) ? icon_modified : icon_path));
      break;
    case TS_BUILD_ICON_IMG_STATUS:
    case TS_BUILD_PATH_DEPTH:
      *treestore_build = g_slist_prepend (*treestore_build, 
					  GUINT_TO_POINTER ((ts_build_cnt == TS_BUILD_ICON_IMG_STATUS) ? 
							    icon_img_status : current_path_depth));
      break;
    default: // txt_fields starts with ICON_PATH_TXT, but this is excluded in the local variant used here.
      *treestore_build = g_slist_prepend (*treestore_build, g_strdup (txt_fields[ts_build_cnt - 3]));
    }
  }


  // --- Preparations for further processing ---


  menu_building->previous_path_depth = current_path_depth;
  free_and_reassign (menu_building->previous_type, g_strdup (txt_fields[TYPE_TXT]));

  if (current_path_depth > menu_building->max_path_depth)
    menu_building->max_path_depth = current_path_depth;
  (menu_building->current_path_depth)++;
}

/* 

   Parses end elements.

*/

static void end_element (GMarkupParseContext G_GNUC_UNUSED  *parse_context,
			 const gchar         G_GNUC_UNUSED  *element_name,
			 gpointer                            menu_building_pnt,
			 GError              G_GNUC_UNUSED **error)
{
  struct menu_building_data *menu_building = (struct menu_building_data *) menu_building_pnt;
  
  if (menu_building->root_menu_finished) // XML after the root-menu is discarded by Openbox.
    return;

  guint current_path_depth = --(menu_building->current_path_depth);

  if (menu_building->loading_stage == ROOT_MENU && current_path_depth == 1)
    menu_building->root_menu_finished = TRUE;
}

/* 

   Parses text from inside an element.

*/

static void element_text (GMarkupParseContext G_GNUC_UNUSED  *parse_context,
			  const gchar                        *text,
			  gsize               G_GNUC_UNUSED   text_len,
			  gpointer                            menu_building_pnt,
			  GError              G_GNUC_UNUSED **error)
{
  struct menu_building_data *menu_building = (struct menu_building_data *) menu_building_pnt;

  // XML after the root-menu is discarded by Openbox.
  if (menu_building->root_menu_finished || !(*(g_strstrip ((gchar *) text))))
    return;

  if (menu_building->treestore_build[TS_BUILD_TYPE] && // Ignore openbox_menu tag.
      streq (menu_building->treestore_build[TS_BUILD_TYPE]->data, "option")) {
    gchar **current_text = (gchar **) &(menu_building->treestore_build[TS_BUILD_VALUE]->data);
    *current_text = g_strdup (text);
    gchar *current_element = menu_building->treestore_build[TS_BUILD_MENU_ELEMENT]->data;
    gchar *current_action = menu_building->current_action;

    if ((streq (current_element, "enabled") || 
	 (streq (current_element, "prompt") && streq_any (current_action, "Exit", "SessionLogout", NULL)))
	&& !streq_any (*current_text, "yes", "no", NULL)) {
      GtkWidget *dialog;
      gchar *dialog_title_txt, *dialog_txt;
      gint result;

      dialog_title_txt = g_strconcat (streq (current_element, "enabled") ? "Enabled" : "Prompt", 
				      " option has invalid value", NULL);
      dialog_txt = g_strdup_printf ("This menu contains a%s <b>%s action</b> that has a%s <b>%s option</b> "
				    "with an <b>invalid value</b>.\nValue: %s\nPlease choose <b>either "
				    "'yes' or 'no'</b> for the option (Closing this dialog sets value to 'no').", 
				    (streq (current_action, "SessionLogout")) ? "" : "n", current_action, 
				    (streq_any (current_action, "Exit", "SessionLogout", NULL)) ? "" : "n", 
				    (streq (current_action, "Execute")) ? "enabled" : "prompt", 
				    *current_text);

      create_dialog (&dialog, dialog_title_txt, GTK_STOCK_DIALOG_ERROR, "yes", "no", NULL, dialog_txt, TRUE);

      // Cleanup
      g_free (dialog_title_txt);
      g_free (dialog_txt);

      result = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      free_and_reassign (*current_text, g_strdup ((result == YES) ? "yes" : "no"));
      activate_change_done ();
    }
  }
}

/* 

   Sets visibility value of menus, pipe menus, items and separators and adds menus and items without labels to a list.

*/

static gboolean elements_visibility (GtkTreeModel  *local_model,
				     GtkTreePath   *local_path,
				     GtkTreeIter   *local_iter,
				     GSList       **menus_and_items_without_label)
{
  gchar *type_txt, *element_visibility_txt;

  gtk_tree_model_get (model, local_iter, 
		      TS_TYPE, &type_txt, 
		      TS_ELEMENT_VISIBILITY, &element_visibility_txt, -1);

  if (streq_any (type_txt, "action", "option", "option block", NULL) || streq (element_visibility_txt, "visible")) {
    // Cleanup
    g_free (type_txt);
    g_free (element_visibility_txt);

    return FALSE;
  }

  gint current_path_depth = gtk_tree_path_get_depth (local_path);
  GtkTreeIter iter_toplevel;
  gchar *element_visibility_txt_toplevel;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gchar *menu_element_txt;

  get_toplevel_iter_from_path (&iter_toplevel, local_path);
  gtk_tree_model_get (model, &iter_toplevel, TS_ELEMENT_VISIBILITY, &element_visibility_txt_toplevel, -1);

  gtk_tree_model_get (local_model, local_iter, TS_MENU_ELEMENT, &menu_element_txt, -1);

  gchar *element_visibility_txt_ancestor = check_if_invisible_ancestor_exists (local_model, local_path);
  gchar *new_element_visibility_txt;

  if (!streq (element_visibility_txt, "invisible unintegrated menu")) {
    if (streq (element_visibility_txt_toplevel, "invisible unintegrated menu"))
      new_element_visibility_txt = "invisible dsct. of invisible unintegrated menu";
    else {
      if (element_visibility_txt_ancestor)
	new_element_visibility_txt = "invisible dsct. of invisible menu";
      else {
	if (menu_element_txt || streq (type_txt, "separator"))
	  new_element_visibility_txt = "visible"; 
	else
	  new_element_visibility_txt = (streq (type_txt, "menu")) ? "invisible menu" : "invisible item";
      }
    }

    gtk_tree_store_set (treestore, local_iter, TS_ELEMENT_VISIBILITY, new_element_visibility_txt, -1);
  }

  /* if the function is called from the "Missing Labels" dialog, the menus_and_items_without_label lists were already 
     built in a previous call of this function. In this case, NULL instead of the menus_and_items_without_label lists 
     was sent as a parameter, so the following if/else statement is not executed. */
  if (menus_and_items_without_label && !menu_element_txt && !streq (type_txt, "separator")) {
    GSList **label_list = &menus_and_items_without_label[(streq (type_txt, "item")) ? ITEMS_LIST : MENUS_LIST];
    gchar *list_elm_txt;
    
    if (streq (type_txt, "item")) {
      if (current_path_depth == 1)
	list_elm_txt = NULL; // Symbolises a toplevel item without label.
      else {
	GtkTreeIter iter_ancestor;
	gchar *menu_id_txt_ancestor;
	
	gtk_tree_model_iter_parent (local_model, &iter_ancestor, local_iter);
	gtk_tree_model_get (local_model, &iter_ancestor, TS_MENU_ID, &menu_id_txt_ancestor, -1);
	
	list_elm_txt = g_strdup_printf ("Child of menu with id '%s'", menu_id_txt_ancestor);

	// Cleanup
	g_free (menu_id_txt_ancestor);
      }
    }
    else
      gtk_tree_model_get (local_model, local_iter, TS_MENU_ID, &list_elm_txt, -1);

    *label_list = g_slist_prepend (*label_list, list_elm_txt);

    // To select an iter, the equivalent row has to be visible.
    if (current_path_depth > 1 &&
	!gtk_tree_view_row_expanded (GTK_TREE_VIEW (treeview), local_path))
      gtk_tree_view_expand_to_path (GTK_TREE_VIEW (treeview), local_path);
    gtk_tree_selection_select_iter (selection, local_iter);
  }

  // Cleanup
  g_free (menu_element_txt);
  g_free (type_txt);
  g_free (element_visibility_txt);
  g_free (element_visibility_txt_toplevel);
  g_free (element_visibility_txt_ancestor);

  return FALSE;
}

/* 

   Creates dialogs that ask about the handling of invisible menus and items.

*/

static void create_dialogs_for_invisible_menus_and_items (guint8             dialog_type, 
							  GtkTreeSelection  *selection, 
							  GSList           **menus_and_items_without_label)
{
  GtkWidget *dialog, *content_area;
  GString *dialog_txt = g_string_new ("");
  gchar *dialog_txt_status_change1 = " are not shown by Openbox. ", 
    *dialog_txt_status_change2 = " If you choose 'Keep status' now, "
    "you can still change their status from inside the program. "
    "Select the menus and either choose 'Edit -> Visualise' from the menu bar or rightclick and choose 'Visualise', "
    "respectively from the context menu.\n\n";
  gint result;

  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);

  GtkWidget *separator[2] = { gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 
			      gtk_separator_new (GTK_ORIENTATION_HORIZONTAL) };

  enum { SEPARATOR_TOP, SEPARATOR_BOTTOM };

  GList *selected_rows;
  
  GtkWidget *menu_grid;
  gchar *cell_txt;

  GSList *menu_elements = NULL;
  GSList *menu_ids = NULL;

  guint number_of_toplevel_items_without_label = 0;

  gboolean valid;
  guint grid_row_cnt = 0;
  guint8 grid_column_cnt, index, lists_cnt;
  GList *g_list_loop;
  GSList *g_slist_loop[2], *g_slist_loop2;
  GtkTreeIter iter_loop, iter_toplevel_loop;
  gchar *menu_element_txt_loop, *menu_id_txt_loop, *element_visibility_txt_loop, *element_visibility_txt_toplevel_loop;

  content_area = create_dialog (&dialog, (dialog_type == UNINTEGRATED_MENUS) ? "Invisible unintegrated menus found" : 
				"Menus/items without label found", GTK_STOCK_DIALOG_INFO, 
				"Keep status", "Visualise", "Delete", (dialog_type == UNINTEGRATED_MENUS) ? 
				"The following menus are defined outside the root menu, "
				"but <b>aren't used inside it</b>:" : 
				"The following <b>menus/items</b> have <b>no label</b>:", FALSE);

  menu_grid = gtk_grid_new ();

  if (dialog_type == UNINTEGRATED_MENUS) {
    valid = gtk_tree_model_iter_nth_child (model, &iter_loop, NULL, gtk_tree_model_iter_n_children (model, NULL) - 1);
    while (valid) {
      gtk_tree_model_get (model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_visibility_txt_loop, -1);

      if (!streq (element_visibility_txt_loop, "invisible unintegrated menu")) {
	// Cleanup
	g_free (element_visibility_txt_loop);
	break;
      }

      gtk_tree_model_get (model, &iter_loop, 
			  TS_MENU_ELEMENT, &menu_element_txt_loop,
			  TS_MENU_ID, &menu_id_txt_loop,
			  -1);

      menu_elements = g_slist_prepend (menu_elements, menu_element_txt_loop);
      menu_ids = g_slist_prepend (menu_ids, menu_id_txt_loop);

      gtk_tree_selection_select_iter (selection, &iter_loop);
      valid = gtk_tree_model_iter_previous (model, &iter_loop);

      // Cleanup
      g_free (element_visibility_txt_loop);
    }

    g_slist_loop[0] = menu_ids;
    g_slist_loop[1] = menu_elements;
  }

  for (lists_cnt = 0; lists_cnt <= dialog_type; lists_cnt++) {
    if (dialog_type == MISSING_LABELS) {
      if (menus_and_items_without_label[lists_cnt]) {
	// Display headlines for the missing labels lists.
	menus_and_items_without_label[lists_cnt] = g_slist_reverse (menus_and_items_without_label[lists_cnt]);

	cell_txt = g_strdup_printf ("\n<b>%s%s:</b> (%s%s shown)\n", 
				    (lists_cnt == MENUS_LIST) ? "Menu" : "Item",
				    (g_slist_length (menus_and_items_without_label[lists_cnt]) > 1) ? "s" : "", 
				    (lists_cnt == MENUS_LIST) ? "Menu ID" : "Location",
				    (g_slist_length (menus_and_items_without_label[lists_cnt]) > 1) ? "s" : "");
	gtk_grid_attach (GTK_GRID (menu_grid), new_label_with_formattings (cell_txt), 0, grid_row_cnt++, 1, 1);

	// Cleanup
	g_free (cell_txt);

	// Display the number of toplevel items without label.
	if (lists_cnt == ITEMS_LIST) {
	  for (g_slist_loop2 = menus_and_items_without_label[ITEMS_LIST]; 
	       g_slist_loop2; 
	       g_slist_loop2 = g_slist_loop2->next) {
	    if (!g_slist_loop2->data)
	      number_of_toplevel_items_without_label++;
	  }
 
	  if (number_of_toplevel_items_without_label) {
	    cell_txt = g_strdup_printf ("%i toplevel item%s%s", 
					number_of_toplevel_items_without_label, 
					(number_of_toplevel_items_without_label > 1) ? "s" : "", 
					(number_of_toplevel_items_without_label == 
					 g_slist_length (menus_and_items_without_label[ITEMS_LIST])) ? "\n" : "");
	    gtk_grid_attach (GTK_GRID (menu_grid), new_label_with_formattings (cell_txt), 0, grid_row_cnt++, 1, 1);

	    // Cleanup
	    g_free (cell_txt);
	    // Done with displaying the number of toplevel items without label, so remove the items from the list.
	    menus_and_items_without_label[lists_cnt] = g_slist_remove_all (menus_and_items_without_label[ITEMS_LIST], 
									   NULL);
	  }
	}
      }
      g_slist_loop[lists_cnt] = menus_and_items_without_label[lists_cnt];
    }
    // Display the unintegrated menus and missing labels lists.
    while (g_slist_loop[lists_cnt]) {
      for (grid_column_cnt = 0; grid_column_cnt <= (dialog_type == UNINTEGRATED_MENUS); grid_column_cnt++) {
	index = ((dialog_type == UNINTEGRATED_MENUS && grid_column_cnt == 1) || 
		 (dialog_type == MISSING_LABELS && lists_cnt == 1));
	cell_txt = g_strdup_printf ("%s%s%s%s  ", (dialog_type == UNINTEGRATED_MENUS && grid_row_cnt == 0) ? "\n" : "",
				    (dialog_type == MISSING_LABELS || 
				     // Unintegrated toplevel menu with menu ID but without label.
				     (grid_column_cnt == 1 && !g_slist_loop[1]->data)) ? 
				    "" : ((grid_column_cnt == 0) ? "<b>Menu ID:</b> " : "<b>Label:</b> "),  
				    (dialog_type == UNINTEGRATED_MENUS && !g_slist_loop[grid_column_cnt]->data) ? 
				    "" : (gchar *) g_slist_loop[index]->data, 
				    (g_slist_loop[index]->next || 
				     (!g_slist_loop[index]->next && dialog_type == MISSING_LABELS && 
				      lists_cnt == MENUS_LIST && menus_and_items_without_label[ITEMS_LIST])) ? 
				    "" : "\n");
	gtk_grid_attach (GTK_GRID (menu_grid), new_label_with_formattings (cell_txt), 
			 grid_column_cnt, grid_row_cnt, 1, 1);

	// Cleanup
	g_free (cell_txt);

	g_slist_loop[index] = g_slist_loop[index]->next;
      }
      grid_row_cnt++;
    }
  }

  g_string_append_printf (dialog_txt, (dialog_type == UNINTEGRATED_MENUS) ? 
			  "\nMenus defined outside the root menu that don't appear inside it%s"
			  "To integrate them, choose 'Visualise' here.%s"
			  "Invisible unintegrated menus and their children are <b>highlighted in blue</b>.\n" :
			  "\nMenus and items without label%s" 
			  "They can be visualised via creating a label for each of them.%s"
			  "Menus and items without label and their children are <b>highlighted in grey</b>.\n", 
			  dialog_txt_status_change1, dialog_txt_status_change2);

  // Cleanup
  if (dialog_type == UNINTEGRATED_MENUS) { 
    g_slist_free_full (menu_elements, (GDestroyNotify) g_free);
    g_slist_free_full (menu_ids, (GDestroyNotify) g_free);
  }
  else {
    g_slist_free_full (menus_and_items_without_label[MENUS_LIST], (GDestroyNotify) g_free);
    g_slist_free_full (menus_and_items_without_label[ITEMS_LIST], (GDestroyNotify) g_free);
  }

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), menu_grid);

  gtk_container_add (GTK_CONTAINER (content_area), separator[SEPARATOR_TOP]);
  gtk_container_add (GTK_CONTAINER (content_area), scrolled_window);
  gtk_container_add (GTK_CONTAINER (content_area), separator[SEPARATOR_BOTTOM]);

  gtk_container_add (GTK_CONTAINER (content_area), new_label_with_formattings (dialog_txt->str));

  // Cleanup
  g_string_free (dialog_txt, TRUE);

  gtk_widget_show_all (dialog);
  gtk_widget_set_size_request (scrolled_window, 570, MIN (gtk_widget_get_allocated_height (menu_grid), 125));

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  switch (result) {
  case KEEP_STATUS:
    break;
  case VISUALISE:
    selected_rows = gtk_tree_selection_get_selected_rows (selection, &model);

    for (g_list_loop = selected_rows; g_list_loop; g_list_loop = g_list_loop->next) {
      gtk_tree_model_get_iter (model, &iter_loop, g_list_loop->data);

      if (dialog_type == UNINTEGRATED_MENUS) {
	gtk_tree_model_get (model, &iter_loop, TS_MENU_ELEMENT, &menu_element_txt_loop, -1);

	// The visibility of subrows is set after the function has been left.
	gtk_tree_store_set (treestore, &iter_loop, TS_ELEMENT_VISIBILITY, 
			    (menu_element_txt_loop) ? "visible" : "invisible menu", 
			    -1);

	// Cleanup
	g_free (menu_element_txt_loop);
      }
      else {
	get_toplevel_iter_from_path (&iter_toplevel_loop, g_list_loop->data);
	gtk_tree_model_get (model, &iter_toplevel_loop,
			    TS_ELEMENT_VISIBILITY, &element_visibility_txt_toplevel_loop, 
			    -1);
 
	gtk_tree_store_set (treestore, &iter_loop, TS_MENU_ELEMENT, "(Newly created label)", -1);
	if (!streq (element_visibility_txt_toplevel_loop, "invisible unintegrated menu"))
	  gtk_tree_store_set (treestore, &iter_loop, TS_ELEMENT_VISIBILITY, "visible", -1);

	// Cleanup
	g_free (element_visibility_txt_toplevel_loop);
      }
    }
    if (dialog_type == MISSING_LABELS)
      gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) elements_visibility, NULL);

    activate_change_done ();

    // Cleanup
    g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
    break;
  case DELETE:
    remove_rows ("load menu");
  }

  gtk_tree_selection_unselect_all (selection);
}

/* 

   Parses a menu file and appends collected elements to the tree view.

*/

void get_tree_row_data (gchar *new_filename)
{
  FILE *file;

  if (!(file = fopen (new_filename, "r"))) {
    gchar *err_txt = g_strdup_printf ("<b>Could not open menu</b>\n<tt>%s</tt><b>!</b>", new_filename);
    show_errmsg (err_txt);

    // Cleanup
    g_free (err_txt);
    g_free (new_filename);

    return;
  }

  gchar *line = NULL;
  size_t length = 0;

  GError *error = NULL;

  struct menu_building_data menu_building = {
    .line_nr =                             1, 
    .treestore_build =                     { NULL }, 
    .current_path_depth =                  1, 
    .previous_path_depth =                 1, 
    .max_path_depth =                      1, 
    .menu_ids =                            NULL,
    .toplevel_menu_ids =                   { NULL }, 
    .icon_creation_error_handling =        UNDEFINED, 
    .current_action =                      NULL, 
    .previous_type =                       NULL, 
    .dep_exe_cmds_have_been_converted =    FALSE, 
    .loading_stage =                       MENUS, 
    .root_menu_finished =                  FALSE
  };

  GMarkupParser parser = { start_element, end_element, element_text, NULL, NULL };
  GMarkupParseContext *parse_context = g_markup_parse_context_new (&parser, 0, &menu_building, NULL);

  guint8 ts_build_cnt;

  while (getline (&line, &length, file) != -1) {
    if (!g_markup_parse_context_parse (parse_context, line, strlen (line), &error)) {
      gchar *pure_errmsg, *escaped_markup_txt;
      GString *full_errmsg = g_string_new ("");

      /* Remove leading and trailing (incl. newline) whitespace from line and 
	 escape all special characters so the markup is used properly. */
      escaped_markup_txt = g_markup_escape_text (g_strstrip (line), -1);
      g_string_append_printf (full_errmsg, "<b>Line %i:</b>\n<tt>%s</tt>\n\n", 
			      menu_building.line_nr, escaped_markup_txt);

      // Cleanup
      g_free (escaped_markup_txt);

      /* Since they are often imprecise, the line and char nr provided by GLib are removed and 
	 replaced by a line nr by the program (see above). */
      if (g_regex_match_simple ("Error", error->message, G_REGEX_ANCHORED, 0))
	// "Error on line 15 char 8: Element..." -> "Element..."
	pure_errmsg = extract_substring_via_regex (error->message, "(?<=: ).*");
      else
	pure_errmsg = g_strdup (error->message);

      // Escape the error message text so the markup of the following text is used properly.
      escaped_markup_txt = g_markup_escape_text (pure_errmsg, -1);

      g_string_append_printf (full_errmsg, "<b><span foreground='#8a1515'>%s!</span>\n\n"
			      "Please&#160;correct&#160;your&#160;menu&#160;file</b>\n<tt>%s</tt>\n"
			      "<b>before reloading it.</b>", escaped_markup_txt, new_filename);

      // Cleanup
      g_free (pure_errmsg);
      g_free (escaped_markup_txt);
      
      show_errmsg (full_errmsg->str);

      // Cleanup
      g_free (new_filename);
      g_string_free (full_errmsg, TRUE);
      g_error_free (error);

      goto parsing_abort;
    }
    (menu_building.line_nr)++;
  }

  for (ts_build_cnt = 0; ts_build_cnt < NUMBER_OF_TS_BUILD_FIELDS; ts_build_cnt++)
    menu_building.treestore_build[ts_build_cnt] = g_slist_reverse (menu_building.treestore_build[ts_build_cnt]);


  // --- Menu file loaded without erros, now (re)set global variables. ---

  clear_global_static_data ();
  menu_ids = g_slist_copy_deep (menu_building.menu_ids, (GCopyFunc) g_strdup, NULL);
  set_filename_and_window_title (new_filename);


  // --- Fill treestore. ---


  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  GtkTreeIter *levels = (GtkTreeIter *) g_malloc (menu_building.max_path_depth * sizeof (GtkTreeIter));

  GSList *menus_and_items_with_inaccessible_icon_image = NULL;
  GSList *menus_and_items_without_label[NUMBER_OF_MISSING_LABEL_LISTS] = { NULL };

  guint number_of_toplevel_menu_ids = 0;
  guint number_of_used_toplevel_root_menus = 0;

  gboolean root_menu_stage = FALSE;
  gboolean add_row = TRUE;
  gboolean menu_or_item_or_separator_at_root_toplevel;

  guint current_level;
  gint row_number = 0;

  GSList *menu_building_loop[NUMBER_OF_TS_BUILD_FIELDS];
  GSList *g_slist_loop;
  GtkTreeIter iter_loop;
  GtkTreePath *path_loop;
  gboolean valid;

  gchar *type_txt_loop;
  gchar *menu_id_txt_loop;

  for (ts_build_cnt = 0; ts_build_cnt < NUMBER_OF_TS_BUILD_FIELDS; ts_build_cnt++)
    menu_building_loop[ts_build_cnt] = menu_building.treestore_build[ts_build_cnt];
  while (menu_building_loop[TS_BUILD_MENU_ELEMENT]) {
    if (streq (menu_building_loop[TS_BUILD_MENU_ID]->data, "root-menu")) {
      number_of_toplevel_menu_ids = gtk_tree_model_iter_n_children (model, NULL);
      root_menu_stage = TRUE;
      
      if (number_of_toplevel_menu_ids) {
	GSList *menu_ids_of_visible_toplevel_menus_defined_outside_root = NULL;
	
	guint invisible_menu_outside_root_index;

	GtkTreeIter iter_swap;

	gchar *menu_element_txt_loop;
	gchar *element_visibility_txt_loop;
	gchar *menu_id_txt_sub_loop;
	guint root_menus_cnt;

	// Generate a list that contains all toplevel menus defined outside the root menu that are visible.

	for (g_slist_loop = menu_building.toplevel_menu_ids[ROOT_MENU]; 
	     g_slist_loop; 
	     g_slist_loop = g_slist_loop->next) {
	  valid = gtk_tree_model_get_iter_first (model, &iter_loop);
	  while (valid) {
	    gtk_tree_model_get (model, &iter_loop, TS_MENU_ID, &menu_id_txt_loop, -1);
	    if (streq (g_slist_loop->data, menu_id_txt_loop)) {
	      gtk_tree_model_get (model, &iter_loop, TS_MENU_ELEMENT, &menu_element_txt_loop, -1);
	      gtk_tree_store_set (treestore, &iter_loop, TS_ELEMENT_VISIBILITY, 
				  (menu_element_txt_loop) ? "visible" : "invisible menu", -1);
	      menu_ids_of_visible_toplevel_menus_defined_outside_root = 
		g_slist_prepend (menu_ids_of_visible_toplevel_menus_defined_outside_root, g_strdup (menu_id_txt_loop));

	      // Cleanup
	      g_free (menu_element_txt_loop);

	      break;
	    }
	    valid = gtk_tree_model_iter_next (model, &iter_loop);

	    // Cleanup
	    g_free (menu_id_txt_loop);
	  }
	}

	/* Move menus that don't show up inside the root menu to the bottom, 
	   keeping their original order, and mark them as invisible. */

	invisible_menu_outside_root_index = number_of_toplevel_menu_ids - 1;
	valid = gtk_tree_model_iter_nth_child (model, &iter_loop, NULL, invisible_menu_outside_root_index);
	while (valid) {
	  gtk_tree_model_get (model, &iter_loop, TS_ELEMENT_VISIBILITY, &element_visibility_txt_loop, -1);
	  if (!element_visibility_txt_loop) {
	    gtk_tree_store_set (treestore, &iter_loop, TS_ELEMENT_VISIBILITY, "invisible unintegrated menu", -1);
	    gtk_tree_model_iter_nth_child (model, &iter_swap, NULL, invisible_menu_outside_root_index--);
	    gtk_tree_store_swap (treestore, &iter_loop, &iter_swap);
	    iter_loop = iter_swap;
	  }
	  valid = gtk_tree_model_iter_previous (model, &iter_loop);

	  // Cleanup
	  g_free (element_visibility_txt_loop);
	}

	/* The order of the toplevel menus depends on the order inside the root menu, 
	   so the menus are sorted accordingly to it. */

	gtk_tree_model_get_iter_first (model, &iter_loop);
	for (g_slist_loop = menu_ids_of_visible_toplevel_menus_defined_outside_root; 
	     g_slist_loop; 
	     g_slist_loop = g_slist_loop->next) {
	  gtk_tree_model_get (model, &iter_loop, TS_MENU_ID, &menu_id_txt_loop, -1);

	  if (!streq (g_slist_loop->data, menu_id_txt_loop)) {
	    for (root_menus_cnt = number_of_used_toplevel_root_menus + 1; // = 1 at first time.
		 root_menus_cnt <= invisible_menu_outside_root_index; 
		 root_menus_cnt++) {
	      gtk_tree_model_iter_nth_child (model, &iter_swap, NULL, root_menus_cnt);
	      gtk_tree_model_get (model, &iter_swap, TS_MENU_ID, &menu_id_txt_sub_loop, -1);
	      if (streq (g_slist_loop->data, menu_id_txt_sub_loop)) {
		// Cleanup
		g_free (menu_id_txt_sub_loop);
		break;
	      }
	      // Cleanup
	      g_free (menu_id_txt_sub_loop);
	    }
	    gtk_tree_store_swap (treestore, &iter_loop, &iter_swap);
	  }

	  gtk_tree_model_iter_nth_child (model, &iter_loop, NULL, ++number_of_used_toplevel_root_menus);

	  // Cleanup
	  g_free (menu_id_txt_loop);
	}
	// Cleanup
	g_slist_free_full (menu_ids_of_visible_toplevel_menus_defined_outside_root, (GDestroyNotify) g_free);
      }

      goto next; // Skip adding a row for this single time.
    }

    type_txt_loop = menu_building_loop[TS_BUILD_TYPE]->data;

    menu_or_item_or_separator_at_root_toplevel = FALSE; // Default
    current_level = GPOINTER_TO_UINT (menu_building_loop[TS_BUILD_PATH_DEPTH]->data) - 1;

    if (root_menu_stage) {
      current_level--;
      if (current_level == 0 && streq_any (type_txt_loop, "menu", "pipe menu", "item", "separator", NULL)) {
	menu_or_item_or_separator_at_root_toplevel = TRUE;
	add_row = TRUE;

	if (streq (type_txt_loop, "menu")) {
	  /* If the current row inside menu_building.treestore_build is a menu with an icon, look for a corresponding 
	     toplevel menu inside the treeview (it will exist if it has already been defined outside the root menu),
	     and if one exists, add the icon data to this toplevel menu. */
	  if (menu_building_loop[TS_BUILD_ICON_IMG]->data) {
	    valid = gtk_tree_model_get_iter_first (model, &iter_loop);
	    while (valid) {
	      gtk_tree_model_get (model, &iter_loop, TS_MENU_ID, &menu_id_txt_loop, -1);
	      if (streq (menu_building_loop[TS_BUILD_MENU_ID]->data, menu_id_txt_loop)) {
		for (ts_build_cnt = 0; ts_build_cnt <= TS_BUILD_ICON_PATH; ts_build_cnt++)
		  gtk_tree_store_set (treestore, &iter_loop, ts_build_cnt, menu_building_loop[ts_build_cnt]->data, -1);
		
		// Cleanup
		g_free (menu_id_txt_loop);
		break;
	      }
	      // Cleanup
	      g_free (menu_id_txt_loop);
	      
	      valid = gtk_tree_model_iter_next (model, &iter_loop);
	    }
	  }

	  if (!menu_building_loop[TS_BUILD_MENU_ELEMENT]->data && 
	      g_slist_find_custom (menu_building.toplevel_menu_ids[MENUS], 
				   menu_building_loop[TS_BUILD_MENU_ID]->data, (GCompareFunc) strcmp)) {
	    add_row = FALSE; // Is a menu defined outside root that is already inside the treestore.
	  }
	}
      }
    }

    if (add_row) {
      GtkTreePath *path;

      gtk_tree_store_insert (treestore, &levels[current_level], (current_level == 0) ? 
			     NULL : &levels[current_level - 1], 
			     (menu_or_item_or_separator_at_root_toplevel) ? row_number : -1);
      
      iter = levels[current_level];
      path = gtk_tree_model_get_path (model, &iter);

      for (ts_build_cnt = 0; ts_build_cnt < TS_BUILD_PATH_DEPTH; ts_build_cnt++)
	gtk_tree_store_set (treestore, &iter, ts_build_cnt, menu_building_loop[ts_build_cnt]->data, -1);

      if (GPOINTER_TO_UINT (menu_building_loop[TS_ICON_IMG_STATUS]->data) && gtk_tree_path_get_depth (path) > 1) {
	/* Add a row reference of a path of a menu, pipe menu or item that has an invalid icon path or 
	   a path that points to a file that contains no valid image data. */
	menus_and_items_with_inaccessible_icon_image = g_slist_prepend (menus_and_items_with_inaccessible_icon_image, 
									gtk_tree_row_reference_new (model, path));
      }

      // Cleanup
      gtk_tree_path_free (path);
    }

    if (menu_or_item_or_separator_at_root_toplevel)
      row_number++;

  next:
    for (ts_build_cnt = 0; ts_build_cnt < NUMBER_OF_TS_BUILD_FIELDS; ts_build_cnt++)
      menu_building_loop[ts_build_cnt] = menu_building_loop[ts_build_cnt]->next;
  }

  g_signal_handler_block (selection, handler_id_row_selected);

  // Show a message if there are invisible menus outside root.
  if (number_of_used_toplevel_root_menus < number_of_toplevel_menu_ids)
    create_dialogs_for_invisible_menus_and_items (UNINTEGRATED_MENUS, selection, NULL);

  gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) elements_visibility, menus_and_items_without_label);

  /* Show a message if there are menus and items without a label (=invisible). */
  if (menus_and_items_without_label[MENUS_LIST] || menus_and_items_without_label[ITEMS_LIST])
    create_dialogs_for_invisible_menus_and_items (MISSING_LABELS, selection, menus_and_items_without_label);

  g_signal_handler_unblock (selection, handler_id_row_selected);


  // --- Finalisation ---


  // Pre-sort options of Execute action and startupnotify, if autosorting is activated.
  if (autosort_options)
    gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) sort_loop_after_sorting_activation, NULL);

  // Expand nodes that contain a broken icon.
  gtk_tree_view_collapse_all (GTK_TREE_VIEW (treeview));
  for (g_slist_loop = menus_and_items_with_inaccessible_icon_image; g_slist_loop; g_slist_loop = g_slist_loop->next) {
    path_loop = gtk_tree_row_reference_get_path (g_slist_loop->data);
    gtk_tree_view_expand_to_path (GTK_TREE_VIEW (treeview), path_loop);
    gtk_tree_view_collapse_row (GTK_TREE_VIEW (treeview), path_loop);

    // Cleanup
    gtk_tree_path_free (path_loop);
  }

  // Notify about a conversion of deprecated execute to command options.
  if (menu_building.dep_exe_cmds_have_been_converted && 
      gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS]))) {
    GtkWidget *dialog, *content_area;
    GtkWidget *chkbt_exe_opt_conversion_notification = gtk_check_button_new_with_label 
      (gtk_menu_item_get_label ((GTK_MENU_ITEM (mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS]))));

    content_area = create_dialog (&dialog, "Conversion of deprecated execute option", GTK_STOCK_DIALOG_INFO, 
				  GTK_STOCK_OK, NULL, NULL, 
				  "This menu contains deprecated 'execute' options, "
				  "they have been converted to 'command' options.\n"
				  "These conversions have not been written back to the menu file yet, "
				  "to do so simply save the menu from inside the program.", FALSE);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chkbt_exe_opt_conversion_notification), TRUE);

    gtk_container_add (GTK_CONTAINER (content_area), chkbt_exe_opt_conversion_notification);

    gtk_widget_show_all (dialog);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mb_view_and_options[NOTIFY_ABOUT_EXECUTE_OPT_CONVERSIONS]),
				    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
								  (chkbt_exe_opt_conversion_notification)));
    gtk_widget_destroy (dialog);

    change_done = TRUE;
  }

  gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview));

  create_list_of_icon_occurrences ();


  // --- Cleanup ---


  g_free (levels);

  g_slist_free_full (menus_and_items_with_inaccessible_icon_image, (GDestroyNotify) gtk_tree_row_reference_free);

 parsing_abort:
  fclose (file);
  g_free (line);
  g_markup_parse_context_free (parse_context);

  // -- treestore_build --

  /* List can contain icon or NULL, 
     g_slist_free_full with g_object_unref would lead to an error message in the latter case. */
  for (g_slist_loop = menu_building.treestore_build[TS_BUILD_ICON_IMG]; g_slist_loop; g_slist_loop = g_slist_loop->next)
    unref_icon ((GdkPixbuf **) &(g_slist_loop->data), FALSE);
  g_slist_free (menu_building.treestore_build[TS_BUILD_ICON_IMG]);

  g_slist_free (menu_building.treestore_build[TS_BUILD_ICON_IMG_STATUS]);

  for (ts_build_cnt = TS_BUILD_ICON_MODIFIED; ts_build_cnt < TS_BUILD_PATH_DEPTH; ts_build_cnt++)
    g_slist_free_full (menu_building.treestore_build[ts_build_cnt], (GDestroyNotify) g_free);

  g_slist_free (menu_building.treestore_build[TS_BUILD_PATH_DEPTH]);

  // -- Other menu_building lists and variables with dyn. alloc. mem. --

  g_slist_free_full (menu_building.menu_ids, (GDestroyNotify) g_free);
  g_slist_free_full (menu_building.toplevel_menu_ids[MENUS], (GDestroyNotify) g_free);
  g_slist_free_full (menu_building.toplevel_menu_ids[ROOT_MENU], (GDestroyNotify) g_free);

  g_free (menu_building.current_action);
  g_free (menu_building.previous_type);
}

/* 

   Lets the user choose a menu xml file for opening.

*/

void open_menu (void)
{
  if (change_done && !unsaved_changes ())
    return;

  GtkWidget *dialog;
  gchar *new_filename;

  create_file_dialog (&dialog, "Open Openbox Menu File");

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    new_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (dialog);
    get_tree_row_data (new_filename);
    row_selected (); // Resets settings for menu- and toolbar.
  }
  else
    gtk_widget_destroy (dialog);
}
