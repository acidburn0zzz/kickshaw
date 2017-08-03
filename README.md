# Kickshaw - A Menu Editor for Openbox

    Copyright (c) 2010-2013        Marcus Schaetzle

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with Kickshaw. If not, see http://www.gnu.org/licenses/.



# REQUIREMENTS

    Dependencies: GTK 3, the code makes use of GNU extensions. Kickshaw is not
    dependent on Openbox, it can be used inside all window managers/desktop
    environments that support GTK applications to create and edit menu files.
    A makefile is included in the source directory. "make" and "make install"
    are sufficient to compile and install this program, there is no configure
    script file which has to be started first.

# IMPORTANT NOTES

    From version 0.5 on this program has all essential plus some additional
    features. This is the first release considered by the author to be stable
    and bug-free enough, as to his knowledge achieved by own reviewing and
    testing.
    Additional input to enhance the program further is of course
    appreciated, since one author alone has always his own "limited view".
    There are some additional features planned for the future (see the TODO
    file for these), but they rather fall into the "luxury" category,
    priority from now on is a stable and as bug-free as possible program.

    Even though the program can cope with whitespace and several tags that
    are nested into each other, the menu xml file has to be syntactically
    correct, or it won't be loaded properly or at all. In this case the
    program gives a detailed error message, if possible.

# FEATURES

    - Dynamic, context sensitive interface
    - The tree view can be customised (show/hide columns, grid etc.)
    - Context menus
    - Editable cells (depending on the type of the cell)
    - (Multirow) Drag and drop
    - The options of an "Execute" action and "startupnotify" option block can
    be auto-sorted to obtain a constant menu structure
    - Search functionality (incl. processing of regular expressions)
    - Multiple selections (hold ctrl/shift key) can be used for mass deletions
    or multirow drag and drop
    - Entering an existing Menu ID is prevented
    - The program informs the user if there are missing menu/item labels,
    invalid icon paths and menus defined outside the root menu that are not
    included inside the latter. By request the program creates labels for
    these invisible menus/items, integrates the orphaned menus into the root
    menu and opens a file chooser dialog so invalid icon paths can be
    corrected.
    - Fast and compact, avoidance of overhead (programmed natively in C,
    only one additional settings file created by the program itself,
    no use of Glade)

# SPECIAL NOTE FOR DRAG AND DROP

    GTK does not support multirow drag and drop, that's why only one row is
    shown as dragged if mulitple rows have been selected for drag and drop.
    There is a workaround for this, but as good as it works for list views
    it doesn't for tree views, so it wasn't implemented here.
