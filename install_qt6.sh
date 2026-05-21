#!/usr/bin/env bash
#
# Install Qt 6.9.3 on Ubuntu 24.04 (for Aurora self-hosted CI runners).
#
# Run as:  sudo ./install_qt6_host_ubuntu2404.sh
#

set -e

# 1. Ensure Python 3.9+ and venv are available
apt-get update
apt-get install -y python3 python3-venv python3-pip

# 2. Create throwaway venv + install aqt + install Qt
python3 -m venv /opt/aqt-venv
/opt/aqt-venv/bin/pip install --no-cache-dir aqtinstall==3.3.0
/opt/aqt-venv/bin/aqt install-qt linux desktop 6.9.3 linux_gcc_64 --outputdir /opt/qt6
rm -rf /opt/aqt-venv  # temporary venv no longer needed

# 3. Verify
ls /opt/qt6/6.9.3/gcc_64/lib/libQt6Core.so.6      # should resolve
/opt/qt6/6.9.3/gcc_64/bin/qmake6 --version         # should print 6.9.3
