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
    python3-venv \
    openssl \
    libssl-dev
    
# Packages for more complex features of VTR that most people will use.
sudo apt-get install -y \
    libtbb-dev

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
    default-jre \
    zlib1g-dev

# Required to build the documentation
sudo apt-get install -y \
    sphinx-common

# Required for code formatting
# NOTE: clang-format-18 may only be found on specific distributions. Only
#       install it if the distribution has this version of clang format.
if apt-cache search '^clang-format-18$' | grep -q 'clang-format-18'; then
    sudo apt-get install -y \
        clang-format-18
else
    echo "clang-format-18 not found in apt-cache. Skipping installation."
fi
