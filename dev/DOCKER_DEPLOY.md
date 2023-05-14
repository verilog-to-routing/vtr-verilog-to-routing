# Building VTR on Docker

## Overview
Docker creates an isolated container on your system so you know that VTR will run without further configurations nor affecting any other work.

Our Docker file sets up this enviroment by installing all necessary Linux packages and applications as well as Perl modules.

## Setup

1. Install docker (Community Edition is free and sufficient for VTR): https://docs.docker.com/engine/install/

2. Clone the VTR project:

    ```
    git clone https://github.com/verilog-to-routing/vtr-verilog-to-routing
    ```

3. CD to the VTR folder and build the docker image:

    ```
    docker build . -t vtrimg
    ```

4. Start docker with the new image:

    ```
    docker run -it -d --name vtr vtrimg
    ```


## Running

1. Attach to the Docker container. Attaching will open a shell on the `/workspace` directory within the container.
The project root directory from the docker build process is copied and placed in the `/workspace` directory.

    ```sh
    # from host computer
    docker exec -it vtr /bin/bash
    ```

2. Ensure that a basic regression test passes:

    ```sh
    # in container
    ./run_reg_test.py vtr_reg_basic
    ```

3. Run and/or modify VTR in the usual way.


## Development Debugging

the container already comes with clang as the default compiler and with scan-build the do statistical analysis on the build
set to `debug` in makefile

run `scan-build make -j4` from the root VTR directory.
to output the html analysis to a specific folder, run `scan-build make -j4 -o /some/folder`

the output is html and viewable in any browser.

