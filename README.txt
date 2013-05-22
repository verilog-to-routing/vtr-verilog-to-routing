This is the development trunk for the Verilog-to-Routing project.  Unlike the 
nicely packaged release that we create, you are working with code in a constant 
state of flux.  You should expect that the tools are not always stable and that 
more work is needed to get the flow to run.

For new developers, please do the tutorial in tutorial/NewDeveloperTutorial.txt.  You
will be directed back here once you ramp up.

Our work in VTR follows a classic svn workflow.  The trunk is supposed to be the 
most current stable version of the project.  Developers checkout a local copy of the code from the 
trunk at the start of development then do regular svn updates to keep in sync 
with the trunk.  When a developer has a tested, working change to put back into 
the trunk, he/she performs an "svn commit" operation.  Unstable code should
remain in the developer's local copy.

We do regular testing of the trunk using an automatic regression testing framework.  You
can see the state of the trunk here: http://canucks.eecg.toronto.edu:8080/waterfall

IMPORTANT:  Outside of special circumstance, a broken build must be fixed at top priority.  
You break the build if your commit breaks any of the automated regression tests.



##############################################################################
# Contributors (Please keep this up-to-date so that we can copy-paste into other locations)
##############################################################################

Professors: Kenneth Kent, Peter Jamieson, Jason Anderson, Vaughn Betz, Jonathan Rose

Graduate Students: Jason Luu, Jeffrey Goeders, Chi Wai Yu, Andrew Somerville, 
Ian Kuon, Alexander Marquardt, Andy Ye, Wei Mark Fang, Tim Liu

Summer Students: Opal Densmore, Ted Campbell, Cong Wang, Peter Milankov, Scott Whitty, Michael Wainberg,
Suya Liu, Miad Nasr, Nooruddin Ahmed, Thien Yu

Companies: Altera Corporation, Texas Instruments


