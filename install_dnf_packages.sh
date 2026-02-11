# Base packages to compile and run basic regression tests
sudo dnf install --refresh -y \
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

# Required for parsing SDC files (see LibSDCParse)
sudo dnf install --refresh -y \
    tcl-devel \
    swig

# Required for graphics
sudo dnf install --refresh -y \
    gtk3-devel \
    libX11

# Required for parmys front-end from https://github.com/YosysHQ/yosys
sudo dnf install --refresh -y \
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

# Required to run the analytical placement flow
sudo dnf install --refresh -y \
    eigen3-devel