sudo apt-get update

# Base packages to compile and run basic regression tests
sudo apt-get install -y \
    make \
    cmake \
    build-essential \
    pkg-config \
    bison \
    flex \
    python3-dev \
    python3-venv
    
# Required for graphics
sudo apt-get install -y \
    libgtk-3-dev \
    libx11-dev

# Required for parmys front-end from https://github.com/YosysHQ/yosys
sudo apt-get install -y \
    build-essential \
    clang \
    bison \
    flex \
    libreadline-dev \
    gawk \
    tcl-dev \
    libffi-dev \
    git \
    graphviz \
    xdot \
    pkg-config \
    python3-dev \
    libboost-system-dev \
    libboost-python-dev \
    libboost-filesystem-dev \
    zlib1g-dev

# Required to build the documentation
sudo apt-get install -y \
    sphinx-common
