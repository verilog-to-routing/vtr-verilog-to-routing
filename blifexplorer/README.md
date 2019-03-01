# BlifExplorer
Odin II visualization tool

The tool visualizes blif files.
It allows to explore the netlist as well as to visualize the step based simulation using random input vectors.

Installation: 

The application requires the Qt5 framework, i.e. in ubuntu: 

	apt-get install qt5-default
	
The application is build using the VTR_ROOT directory make file using the following command:

	make CMAKE_PARAMS="-DWITH_BLIFEXPLORER=on"

Once the project is compiled the application will reside in the blifexplorer directory. 

