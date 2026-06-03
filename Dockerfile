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
# install_apt_packages.sh uses bash control flow (--dev) instead of sed parsing
    && apt-get install -y --no-install-recommends sudo \
    && bash install_apt_packages.sh --dev \
# Additional packages not listed in install_apt_packages.sh
    && apt-get -y install --no-install-recommends \
    wget \
    ninja-build \
    python3-pip \
    time \
# Install python packages
    && pip install -r requirements.txt \
# Cleanup
    && apt-get autoclean && apt-get clean && apt-get -y autoremove \
    && rm -rf /var/lib/apt/lists/*
# Build VTR
RUN rm -rf build && make -j$(nproc) && make install
# Container's default launch command
SHELL ["/bin/bash", "-c"]