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

# Required for yosys front-end
sudo apt-get install -y \
    clang \
    tcl-dev \
    libreadline-dev

# Required to build the documentation
sudo apt-get install -y \
    sphinx-common
