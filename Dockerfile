FROM ubuntu:20.04
# set out workspace
ENV WORKSPACE=/workspace
RUN mkdir -p ${WORKSPACE}
VOLUME ${WORKSPACE}
WORKDIR ${WORKSPACE}
# Set environment variables
ARG GIT_SSL_NO_VERIFY=1
ARG DEBIAN_FRONTEND=noninteractive
# Install and cleanup is done in one command to minimize the build cache size
RUN apt-get update -qq \
    && apt-get -y install --no-install-recommends \
# Additional packages not listed in install_apt_packages.sh
    wget \
    git \
    ninja-build \
    libeigen3-dev \
    libtbb-dev \
    python3-pip \
# Clone VTR repo
    && git clone https://github.com/verilog-to-routing/vtr-verilog-to-routing.git . \
# Extract package names from install_apt_packages.sh
    && sed '/sudo/d' install_apt_packages.sh | sed '/#/d' | sed 's/ \\//g' | sed '/^$/d' | sed '/^[[:space:]]*$/d' \
# Install packages
    | xargs apt-get -y install --no-install-recommends \
# Cleanup
    && apt-get autoclean && apt-get clean && apt-get -y autoremove \
    && rm -rf /var/lib/apt/lists/*
# Install python packages
RUN pip install -r requirements.txt
# Build VTR
RUN make \
&& make install
# Container's default launch command
CMD [ "/bin/bash" ]