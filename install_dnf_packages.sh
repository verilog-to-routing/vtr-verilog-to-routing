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
    tcl8-devel \
    swig

# Required for graphics
# Qt6 (Core/Gui/Widgets via qt6-qtbase-devel, which also provides qmake6/moc/
# uic/rcc and the CMake config) plus its GL / xkb runtime deps.
sudo dnf install --refresh -y \
    qt6-qtbase-devel \
    libxkbcommon-devel \
    mesa-libGL-devel \
    mesa-libEGL-devel \
    libglvnd-opengl \
    mesa-dri-drivers

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