FROM ubuntu:24.04
ARG DEBIAN_FRONTEND=noninteractive
# set out workspace
ENV WORKSPACE=/workspace
RUN mkdir -p ${WORKSPACE}
WORKDIR ${WORKSPACE}
COPY . ${WORKSPACE}
# Required to bypass Python's protection on system-wide package installations in Ubuntu 23.04+.
# This allows pip to install packages globally without using a virtual environment.
ENV PIP_BREAK_SYSTEM_PACKAGES=1
# Install and cleanup is done in one command to minimize the build cache size
RUN apt-get update -qq \
# extract run-time package names from install_apt_packages.sh via sed parsing
    && sed '/sudo/d' install_apt_packages.sh | sed '/#/d' | sed '/packages_to_install/d' | sed '/)/d' | sed '/if\s.*then$/d' | sed '/if\s*\[\[/d' | sed '/else$/d' | sed '/fi$/d' | sed '/echo\s/d' | sed '/install_dev/d' | sed '/for /d' | sed '/done/d' | sed '/verilator/d' | sed 's/ \\//g' | sed '/^$/d' | sed '/^[[:space:]]*$/d' | sed 's/\s//g' \
# install run-time packages needed to run vtr
    | xargs apt-get -y install --no-install-recommends \
# additional packages for this docker image, including developer packages (--dev)
    && apt-get -y install --no-install-recommends \
    wget \
    ninja-build \
    python3-pip \
    time \
    verilator \
# Install python packages
    && pip install -r requirements.txt \
# Cleanup
    && apt-get autoclean && apt-get clean && apt-get -y autoremove \
    && rm -rf /var/lib/apt/lists/*
# Build VTR
RUN rm -rf build && make -j$(nproc) && make install
# Container's default launch command
SHELL ["/bin/bash", "-c"]
