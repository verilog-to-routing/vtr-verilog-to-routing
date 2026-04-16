# Building VTR


## Setting up Your Environment

If you cloned the repository you will need to set up the git submodules (if you downloaded and extracted a release, you can skip this step):

    git submodule init
    git submodule update

### Linux (Debian / Ubuntu)

VTR requires several system packages.  From the top-level directory, run the following script to install the required packages on a modern Debian or Ubuntu system:

    ./install_apt_packages.sh

### Linux (Fedora / RHEL)

For Fedora or RHEL-based systems:

    ./install_dnf_packages.sh

### macOS

VTR builds on macOS (Apple Silicon and Intel) using Homebrew.  First, install [Homebrew](https://brew.sh) and the Xcode Command Line Tools if you haven't already:

    xcode-select --install

Then install the required packages:

    ./install_brew_packages.sh

The build system automatically detects macOS and configures Homebrew's bison, flex, Qt6, and other dependencies. No manual path configuration is required.

### Python Packages

You will also need several Python packages.  You can optionally install and activate a Python virtual environment so that you do not need to modify your system Python installation:

    make env
    source .venv/bin/activate

Then to install the Python packages:

    pip install -r requirements.txt

**Note:** If you chose to install the Python virtual environment, you will need to remember to activate it on any new terminal window you use, before you can run the VTR flow or regressions tests (`source .venv/bin/activate`).

## Building

From the top-level, run:

    make

which will build all the required tools.

For parallel builds (recommended):

    # Linux
    make -j$(nproc)

    # macOS
    make -j$(sysctl -n hw.ncpu)

### Supported Platforms

The complete VTR flow has been tested on:
- 64-bit Linux (Debian, Ubuntu, Fedora)
- macOS (Apple Silicon and Intel, with Homebrew)

*Full information about building VTR, including setting up required system packages and Python packages, can be found in [Optional Build Information](doc/src/vtr/optional_build_info.md) page.*

Please [let us know](doc/src/contact.md) your experience with building VTR so that we can improve the experience for others.

The tools included official VTR releases have been tested for compatibility.
If you download a different version of those tools, then those versions may not be mutually compatible with the VTR release.

## Verifying Installation

To verify that VTR has been installed correctly run::

    ./vtr_flow/scripts/run_vtr_task.py ./vtr_flow/tasks/regression_tests/vtr_reg_basic/basic_timing

The expected output is::
    
    k6_N10_mem32K_40nm/single_ff            OK
    k6_N10_mem32K_40nm/single_ff            OK
    k6_N10_mem32K_40nm/single_wire          OK
    k6_N10_mem32K_40nm/single_wire          OK
    k6_N10_mem32K_40nm/diffeq1              OK
    k6_N10_mem32K_40nm/diffeq1              OK
    k6_N10_mem32K_40nm/ch_intrinsics                OK
    k6_N10_mem32K_40nm/ch_intrinsics                OK

