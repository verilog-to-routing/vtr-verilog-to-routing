.. _edit_vpr_ui:

VPR UI and Graphics
--------------

VPR's UI is created with GTK, and actively maintained/edited through the use of Glade and a main.ui file. We prefer to not use code initializations of Gtk Buttons/UI objects, and instead make them with Glade. 
This allows for better organized menus and visual editing of the UI. Please consult the attached guide for Glade: <Link Glade/Gtk quickstart> (WIP as of August 24th, 2022). 

When connecting a button to its function, place it in an appropriate function depending on the drop down menu it will go in. Button setups are done in ui_setup.cpp/h and callback functions are in draw_toggle_functions.cpp/h.

VPR Graphics are drawn using the EZGL graphics library, which is a wrapper around the GTK graphics library (which is used for the UI). EZGL Documentation is found here: http://ug251.eecg.utoronto.ca/ece297s/ezgl_doc/index.html and GTK documentation is found here: https://docs.gtk.org/gtk3/

Make sure to test the UI when you edit it. Ensure that the graphics window opens (using the --disp on command) and click around the UI to ensure the buttons still work. Test all phases (Placement -> Routing) as the UI changes between them. 
