.. _compiling_vpr:

Compiling VPR
-------------

Compiling VPR on Linux, Solaris and Mac
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If your compiler of choice is gcc and you are running a Unix-based system, you can compile VPR simply by typing make in the directory containing VPR’s source code and makefile.
The makefile contains the following options located at the top of the makefile:

* ``ENABLE_GRAPHICS`` (default = true) enables or disables VPR’s X11-based graphics.  
  If you do not have the X11 headers installed or you would prefer to compile VPR without graphics, set this to false.

* ``BUILD_TYPE`` (default = release) turns on optimization when set to release, and turns on debug information when set to debug.

* ``MAC_OS`` (default = false) changes the directory in which X11 is found to the default location for Mac when set to true.
  Irrelevant unless ENABLE_GRAPHICS is set to true.

* ``COMPILER`` (default = g++) sets the compiler.

* ``OPTIMIZATION_LEVEL_OF_RELEASE_BUILD`` (default = -O3) sets the level of optimi-zation for the release build.
  Only change this from -O3 if you are using a compiler other than gcc and you think you will get better optimization with another level.

If ``ENABLE_GRAPHICS`` is set to true, VPR requires X11 and Xft to compile.
In Ubuntu, type make packages or ``sudo apt-get install libx11-dev libxft-dev`` to install.
Please look online for information on how to install X11 on other Linux distributions.
If you have the libraries installed in a non-default location on Linux, search the makefile for -L/usr/lib/X11 and and add/change the file path.
If, during compilation, you get an error that type ``XPointer`` is not defined, uncomment the ``typedef char *XPointer`` line in graphics.c (many X Windows implementations do not define the XPointer type).

Compiling VPR on Windows
~~~~~~~~~~~~~~~~~~~~~~~~

A Visual Studio project file, VPR.vcxproj, is included in VPR’s parent directory.
To input command-line arguments, right-click on VPR in the Solution Explorer and select Properties (or go to VPR Properties in the Project menu while viewing a source file from VPR), then select Configuration Properties -> Debugging.
Enter the architecture filename, circuit filename and any optional parameters you wish in the Command Arguments box.  

If you receive an error “Cannot open the pdb file”, go to Options and Settings in the Debug menu, and select Debugging -> Symbols.
Make sure that Microsoft Symbol Servers is checked.
You may also need to run Visual Studio as an administrator.

If you prefer to use a Linux-based environment for development in Windows, then we recommend using the `Cygwin <https://www.cygwin.com>`_ package to compile VPR.
