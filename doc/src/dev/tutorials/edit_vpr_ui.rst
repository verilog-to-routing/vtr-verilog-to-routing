.. _edit_vpr_ui:

Editing VPR UI
--------------

VPR's UI is created with GTK, and actively maintained/edited through the use of Glade and a main.ui file. We prefer to not use code initializations of Gtk Buttons/UI objects, and instead make them with Glade. 
This allows for better organized menus and visual editing of the UI. Please consult the attached guide for Glade: <Link Glade/Gtk quickstart> (WIP as of August 8th, 2022). 

When connecting a button to its function, place it in an appropriate function depending on the drop down menu it will go in. Button setups are done in ui_setup.cpp/h and callback functions are in draw_toggle_functions.cpp/h.