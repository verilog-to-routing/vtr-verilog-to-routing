README: Odin II visualization tool

The tool visualizes blif files created by Odin II. It allows to explore the netlist as well as to visualize the step based simulation using random input vectors.

Installation: 

The application requires the qt framework. The easiest way to compile and run the program is to install qtCreator: 

	apt-get install qtcreator

The folder is structured into three sections: 

blifexplorer/src: the source files as well as graphics etc. 
blifexplorer/build: the compiled files and the executable, which allows to run the program without starting qtCreator. 
blifexplorer/debug: used in the qtCreator for the debug compilation process. 

In qtCreator please use the "File->Open File or Project..." dialog to open the project file: blifexplorer/src/blifExplorer.pro

During the first start it is necessary to set the build and debug environment as qtCreator does not allow relative paths.
In the project setup window, which pops up during the first start please choose "Manual" in the "Create Build Configuration" drop down menu.
Change the configuration to the following values: 

Qt in PATH release: blifexplorer/build/
Qt in PATH debug:   blifexplorer/debug/

Once the project is open, the application can be started using the big green button on the bottom left side. 


Summary: 
-install qtcreator: apt-get install qtcreator
-open blifexplorer/src/blifExplorer.pro
-Create Build Configuration: Manual
-Qt in PATH release: blifexplorer/build/
-Qt in PATH debug:   blifexplorer/debug/

