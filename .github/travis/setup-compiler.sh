#!/usr/bin/env bash

set -x
set -e

eval "${MATRIX_EVAL}"

if [[ -z "$CC" ]]; then
	echo "No \$CC value set! Can not install the compiler!"
	exit 1
else
	echo "Using $CC compiler"
fi
if [[ -z "$CXX" ]]; then
	echo "No \$CXX value set! Can not install the compiler!"
	exit 1
else
	echo "Using $CXX compiler"
fi

UBUNTU_DISTRO=$(lsb_release -c -s)

case $CC in
	clang*)
		echo "$CC is provided by https://apt.llvm.org"
		wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
		cat >> /etc/apt/sources.list <<EOF
deb http://apt.llvm.org/${UBUNTU_DISTRO} llvm-toolchain-${UBUNTU_DISTRO}-$(echo $CC | sed -e's/clang-//') main
EOF
		;;
	*)
		echo "$CC is provided by default repositories."
		;;
esac

mkdir -p /etc/apt/apt.conf.d
sudo tee /etc/apt/apt.conf.d/99allow_unauth <<EOF
APT {
	Ignore {
		"gpg-pubkey";
	}
	Get {
		AllowUnauthenticated "1";
		Assume-Yes "1";
		Install-Recommends "0";
		Install-Suggests "0";
	}
};
EOF
apt-config dump

sudo -E apt-get --quiet --force-yes update
sudo -E apt-get --quiet --force-yes install $CC $CXX
