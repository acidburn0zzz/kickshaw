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

#ifndef __find_h
#define __find_h

extern GtkTreeModel *model;
extern GtkTreeView *treeview;
extern GtkTreeIter iter;

extern GtkTreeViewColumn *columns[];
#define TREEVIEW_COLUMN_OFFSET NUMBER_OF_TS_ELEMENTS - NUMBER_OF_COLUMNS

extern GtkWidget *mb_view_and_options[];

extern GtkWidget *find_grid;
extern GtkWidget *find_button_entry_row[];
extern GtkWidget *find_entry;
extern GtkWidget *find_in_columns[], *find_in_all_columns;
extern GtkWidget *find_match_case, *find_regular_expression; 

extern const gchar *search_term_str;

extern GList *rows_with_found_occurrences;

extern gint handler_id_find_in_columns[];

extern void row_selected (void);

#endif
