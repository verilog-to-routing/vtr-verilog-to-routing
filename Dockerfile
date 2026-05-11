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
# Extract package names from install_apt_packages.sh (list-only mode
# avoids fragile sed-parsing of the script's shell control flow).
    && VTR_LIST_PACKAGES_ONLY=1 bash install_apt_packages.sh \
# Install packages
    | xargs apt-get -y install --no-install-recommends \
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
# Install Qt 6.9.3 (no sudo needed in Docker; logic mirrors install_apt_packages.sh)
RUN pip install aqtinstall \
    && aqt install-qt linux desktop 6.9.3 linux_gcc_64 --outputdir /opt/qt6 --modules qtshadertools
ENV CMAKE_PREFIX_PATH=/opt/qt6/6.9.3/gcc_64
# Build VTR
RUN rm -rf build && make -j$(nproc) && make install
# Container's default launch command
SHELL ["/bin/bash", "-c"]
