FROM ubuntu:trusty as builder

RUN apt-get update 
RUN apt-get install -y \
    software-properties-common

# add auto gpg key other lauchpad ppa
RUN add-apt-repository ppa:nilarimogard/webupd8
RUN apt-get update && apt-get install -y \
    launchpad-getkeys

# add llvm PPA
RUN printf "\n\
deb http://ppa.launchpad.net/george-edison55/precise-backports/ubuntu precise main \n\
deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu trusty main \n\
deb https://apt.llvm.org/precise llvm-toolchain-precise-3.6 main \n\
deb https://apt.llvm.org/trusty llvm-toolchain-trusty-6.0 main \n\
deb https://apt.llvm.org/trusty llvm-toolchain-trusty-7 main \n\
deb https://apt.llvm.org/trusty llvm-toolchain-trusty-8 main \n\
" >> /etc/apt/sources.list

# grab llvm keys
RUN launchpad-getkeys

RUN apt-get update
RUN apt-get install -y \
    ninja \
    libssl-dev \
    autoconf \
    automake \
    bash \
    bison \
    binutils \
    binutils-gold \
    build-essential \
    ctags \
    curl \
    doxygen \
    flex \
    fontconfig \
    gdb \
    git \
    gperf \
    libcairo2-dev \
    libgtk-3-dev \
    libevent-dev \
    libfontconfig1-dev \
    liblist-moreutils-perl \
    libncurses5-dev \
    libx11-dev \
    libxft-dev \
    libxml++2.6-dev \
    perl \
    python \
    python-lxml \
    texinfo \
    time \
    valgrind \
    zip \
    qt5-default \
    clang-format-7 \
    g++-5 \
    gcc-5 \
    g++-6 \
    gcc-6 \
    g++-7 \
    gcc-7 \
    g++-8 \
    gcc-8 \
    g++-9 \
    gcc-9 \
    clang-6.0 \
    clang-8

# install CMake
WORKDIR /tmp
ENV CMAKE=cmake-3.17.0
RUN curl -s https://cmake.org/files/v3.17/${CMAKE}.tar.gz | tar xvzf -
RUN cd ${CMAKE} && ./configure && make && make install

# set out workspace
ENV WORKSPACE=/workspace
RUN mkdir -p ${WORKSPACE}
VOLUME ${WORKSPACE}
WORKDIR ${WORKSPACE}

CMD [ "/bin/bash" ]