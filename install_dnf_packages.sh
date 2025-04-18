sudo dnf upgrade --refresh

# Base packages to compile and run basic regression tests
sudo dnf install -y \
    make \
    cmake \
    automake \
    gcc \
    gcc-c++ \
    kernel-devel \
    pkg-config \
    bison \
    flex \
    python3-devel \
    tbb-devel
# Required for graphics
sudo dnf install -y \
    gtk3-devel \
    libX11

# Required for parmys front-end from https://github.com/YosysHQ/yosys
sudo dnf install -y \
    make \
    automake \
    gcc \
    gcc-c++ \
    kernel-devel \
    clang \
    bison \
    flex \
    readline-devel \
    gawk \
    tcl-devel \
    libffi-devel \
    git \
    graphviz \
    python-xdot \
    pkg-config \
    python3-devel \
    boost-system \
    boost-python3 \
    boost-filesystem \
    zlib-ng-devel

# Required to build the documentation
sudo dnf install -y \
    python3-sphinx \
    python-sphinx-doc

# Required to run the analytical placement flow
sudo dnf install -y \
    eigen3-devel