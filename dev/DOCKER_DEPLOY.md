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

1. Verfiy that VTR has been installed correctly:

    ```sh
    # in container
    ./vtr_flow/scripts/run_vtr_task.py regression_tests/vtr_reg_basic/basic_timing
    ```

    The expected output is:

    ```
    k6_N10_mem32K_40nm/single_ff            OK
    k6_N10_mem32K_40nm/single_ff            OK
    k6_N10_mem32K_40nm/single_wire          OK
    k6_N10_mem32K_40nm/single_wire          OK
    k6_N10_mem32K_40nm/diffeq1              OK
    k6_N10_mem32K_40nm/diffeq1              OK
    k6_N10_mem32K_40nm/ch_intrinsics                OK
    k6_N10_mem32K_40nm/ch_intrinsics                OK
    ```

2. Run and/or modify VTR in the usual way.

