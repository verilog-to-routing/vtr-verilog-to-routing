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

    make -j$(nproc)

### Graphics (GUI)

VPR has an optional Qt6-based graphical interface. A plain `make` builds the GUI
**only if a suitable Qt6 (>= 6.9.3) is installed on the system**; otherwise VPR
is built headless. The 6.9.3 floor exists because earlier Qt6 releases have QRhi
rendering bugs.

If your system Qt6 is missing or older than 6.9.3, you can install a local copy
(no root required) with:

    ./dev/ensure_qt6_sdk.sh

> **Note:** `ensure_qt6_sdk.sh` is a temporary measure. From Ubuntu 26.04 onward
> the system Qt6 will satisfy the >= 6.9.3 requirement, so the system Qt will be
> used directly and `ensure_qt6_sdk.sh` will become obsolete.

Two convenience targets make the choice explicit:

    make ensure-gui        # build vpr WITH the GUI
    make ensure-headless   # build vpr WITHOUT the GUI

- `make ensure-gui` first runs `dev/ensure_qt6_sdk.sh` (provisioning a Qt6 SDK
  if the system one is missing/too old, and aborting the build if that fails),
  then configures with graphics enabled and builds `vpr`.
- `make ensure-headless` configures with graphics disabled and builds `vpr`; no
  Qt is required.

### Supported Platforms

The complete VTR flow has been tested on:
- 64-bit Linux (Debian, Ubuntu, Fedora)

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

