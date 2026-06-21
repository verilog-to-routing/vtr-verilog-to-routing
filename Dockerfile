FROM ubuntu:24.04
ARG DEBIAN_FRONTEND=noninteractive
# set out workspace
ENV WORKSPACE=/workspace
RUN mkdir -p ${WORKSPACE}
WORKDIR ${WORKSPACE}
COPY . ${WORKSPACE}
ENV VIRTUAL_ENV=${WORKSPACE}/.venv
ENV PATH="${VIRTUAL_ENV}/bin:${PATH}"
# Install and cleanup is done in one command to minimize the build cache size
RUN apt-get update -qq \
# Extract package names from install_apt_packages.sh
    && sed '/sudo/d' install_apt_packages.sh | sed '/#/d' | sed '/packages_to_install/d' | sed '/)/d' | sed '/if\s.*then$/d' | sed '/else$/d' | sed '/fi$/d' | sed '/echo\s/d' | sed '/install_dev/d' | sed '/for /d' | sed '/^done$/d' | sed 's/ \\//g' | sed '/^$/d' | sed '/^[[:space:]]*$/d' | sed 's/\s//g' \
# Install packages
    | xargs apt-get -y install --no-install-recommends \
# Additional packages not listed in install_apt_packages.sh
    && apt-get -y install --no-install-recommends \
    wget \
    ninja-build \
    python3-pip \
    python3-venv \
    time \
    verilator \
# Install python packages
    && python3 -m venv ${VIRTUAL_ENV} \
    && pip install -r requirements.txt \
    && echo "[ -f ${VIRTUAL_ENV}/bin/activate ] && . ${VIRTUAL_ENV}/bin/activate" > /etc/profile.d/vtr-venv.sh \
    && echo "[ -f ${VIRTUAL_ENV}/bin/activate ] && . ${VIRTUAL_ENV}/bin/activate" >> /etc/bash.bashrc \
# Cleanup
    && apt-get autoclean && apt-get clean && apt-get -y autoremove \
    && rm -rf /var/lib/apt/lists/*
# Provision the Qt6 SDK exactly like CI: dev/ensure_qt6_sdk.sh installs it
# (no sudo) into the repo-local prefix, which CMake auto-detects — so no
# CMAKE_PREFIX_PATH is needed.
RUN bash dev/ensure_qt6_sdk.sh
# Build VTR
RUN rm -rf build && make -j$(nproc) && make install
# Container's default launch command
SHELL ["/bin/bash", "-c"]
