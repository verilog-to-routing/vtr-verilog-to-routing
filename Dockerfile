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
# Extract package names from install_apt_packages.sh
    && sed '/sudo/d' install_apt_packages.sh | sed '/#/d' | sed '/packages_to_install/d' | sed '/)/d' | sed '/if\s.*then$/d' | sed '/else$/d' | sed '/fi$/d' | sed '/echo\s/d' | sed 's/ \\//g' | sed '/^$/d' | sed '/^[[:space:]]*$/d' | sed 's/\s//g' \
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
# Install aqtinstall (no root needed for pip, but in Docker we're building the image)
RUN pip install aqtinstall

# Install Qt 6.9.3 into /opt/qt6 — isolated from system Qt5.
# 6.9.3 is the minimum required version: earlier Qt6 releases have internal bugs
# in the QRhi subsystem that cause rendering failures on our target platforms.
RUN aqt install-qt linux desktop 6.9.3 linux_gcc_64 \
      --outputdir /opt/qt6
# Point CMake to the aqt-installed Qt so find_package(Qt6) resolves without extra -D flags.
ENV CMAKE_PREFIX_PATH=/opt/qt6/6.9.3/linux_gcc_64
# Build VTR
RUN rm -rf build && make -j$(nproc) && make install
# Container's default launch command
SHELL ["/bin/bash", "-c"]
