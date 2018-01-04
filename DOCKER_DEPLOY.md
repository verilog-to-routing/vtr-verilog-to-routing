Overview
========

Docker creates an isolated container on your system so you know that VTR will run without further configurations nor affecting any other work.

Our Docker file sets up this enviroment by installing all necessary Linux packages and applications as well as Perl modules.

Additionally, Cloud9 is installed, which enables the remote management of your container through browser. With Cloud9, VTR can be started easier (and even modified and recompiled) without the need to logging into a terminal. If the Cloud9 endpoint is published outside your LAN, you can also execute VTR remotely or share your screen with other users.


Setup
=====

Install docker (Community Edition is free and sufficient for VTR): https://docs.docker.com/engine/installation/

Clone the VTR project:

`git clone https://github.com/verilog-to-routing/vtr-verilog-to-routing`

CD to the VTR folder and build the docker image:

`docker build . -t vtrimg`

Start docker with the new image and connect the current volume with the workspace volume of the container:

`sudo docker run -it -d -p <port-to-open-on-host>:8080 -v <absolute-path-to-VTR-folder>:/workspace vtrimg`


Running
=======

Open a browser (Google Chrome for example) and navigate to your host's url at the port you opened up. For example:
http://192.168.1.30:8080

First, use one of the terminals and compile VTR:
make && make installation/

Second, ensure that a basic regression test passes:
./run_reg_test.pl vtr_reg_basic

Third, run and/or modify VTR in the usual way.

Developpement Debugging
=======================
the container already comes with clang as the default compiler and with scan-build the do statistical analysis on the build
set to `debug` in makefile

run `scan-build make -j4` from the root VTR directory.
to output the html analysis to a specific folder, run `scan-build make -j4 -o /some/folder`

the output is html and viewable in any browser.

