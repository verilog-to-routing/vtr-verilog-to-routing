This is the development trunk for the Verilog-to-Routing project.  Unlike the
nicely packaged release that we create, you are working with code in a constant
state of flux.  You should expect that the tools are not always stable and that
more work is needed to get the flow to run.

For new developers, please do the tutorial in tutorial/NewDeveloperTutorial.txt.
You will be directed back here once you ramp up.

Our work in VTR follows a classic centralized repository (svn-like) workflow.
The 'master' branch is supposed to be the most current stable version of the project.
Developers checkout a local copy of the code at the start of development, then do
regular updates (e.g. git pull) to keep in sync with the GitHub master.  When a
developer has a tested, working change to put back into the trunk, he/she performs a
"git push" operation. Unstable code should remain in the developer's local copy.

We do regular testing of the trunk using an automatic regression testing
framework. You can see the state of the trunk here:
http://islanders.eecg.utoronto.ca:8080/waterfall
QoR tracking links can be found here:
http://islanders.eecg.utoronto.ca:8080/

IMPORTANT:  Outside of special circumstance, a broken build must be fixed at top
priority. You break the build if your commit breaks any of the automated
regression tests.



##############################################################################
# Contributors (Please keep this up-to-date)
##############################################################################

Professors: Kenneth Kent, Peter Jamieson, Jason Anderson, Vaughn Betz,
Jonathan Rose

Graduate Students: Jason Luu, Jeffrey Goeders, Chi Wai Yu, Andrew Somerville,
Ian Kuon, Alexander Marquardt, Andy Ye, Wei Mark Fang, Tim Liu, Charles Chiasson,
Kevin Murray

Summer Students: Opal Densmore, Ted Campbell, Cong Wang, Peter Milankov,
Scott Whitty, Michael Wainberg, Suya Liu, Miad Nasr, Nooruddin Ahmed, Thien Yu,
Long Yu Wang, Matthew J.P. Walker, Amer Hesson, Sheng Zhong, Hanqing Zeng

Companies: Altera Corporation, Texas Instruments


