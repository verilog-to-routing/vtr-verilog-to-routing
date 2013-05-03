#!/bin/sh
set -e

echo "Removing old vtr_release directory"
rm -rf vtr_release
rm -rf vtr_release.tar
rm -rf vtr_release.tar.gz

echo "Make new vtr_release directory"
mkdir vtr_release
mkdir vtr_release/ODIN_II
mkdir vtr_release/ODIN_II/OBJ
mkdir vtr_release/abc_with_bb_support
mkdir vtr_release/libarchfpga
mkdir vtr_release/pcre
mkdir vtr_release/ace2
mkdir vtr_release/printhandler
mkdir vtr_release/quick_test
mkdir vtr_release/vpr

echo "Clean builds"
make clean

echo "Build ODIN II"
cp ODIN_II/*.txt vtr_release/ODIN_II
cp -r ODIN_II/Makefile vtr_release/ODIN_II
cp -r ODIN_II/SRC vtr_release/ODIN_II
cp -r ODIN_II/REGRESSION_TESTS vtr_release/ODIN_II
cp -r ODIN_II/FULL_REGRESSION_TESTS vtr_release/ODIN_II
cp -r ODIN_II/USEFUL_TOOLS vtr_release/ODIN_II

echo "Build ABC"
cp -r abc_with_bb_support/* vtr_release/abc_with_bb_support

echo "Build printhandler"
cp -r printhandler vtr_release/
rm -rf vtr_release/printhandler/Debug

echo "Build pcre"
cp -r pcre vtr_release/
rm -rf vtr_release/pcre/Debug

echo "Build ace2"
cp -r ace2 vtr_release/

echo "Build libarchfpga"
cp -r libarchfpga/arch vtr_release/libarchfpga
cp -r libarchfpga/include vtr_release/libarchfpga
cp libarchfpga/*.xml vtr_release/libarchfpga
cp libarchfpga/Makefile vtr_release/libarchfpga
cp libarchfpga/*.c vtr_release/libarchfpga
cp libarchfpga/libarchfpga.vcxproj vtr_release/libarchfpga
cp libarchfpga/libarchfpga.vcxproj.filters vtr_release/libarchfpga

echo "Build regtest"
cp -r vtr_flow vtr_release/vtr_flow
rm -rf vtr_release/vtr_flow/tasks/*/run*
rm -rf vtr_release/vtr_flow/tasks/*/*/run*
rm -rf vtr_release/vtr_flow/tasks/*/*/*/run*
cp quick_test/abc_test.blif vtr_release/quick_test
cp quick_test/depth_split.v vtr_release/quick_test
cp quick_test/depth_split.xml vtr_release/quick_test
cp quick_test/sample_arch.xml vtr_release/quick_test
cp quick_test/vpr_test.blif vtr_release/quick_test

echo "Build vpr"
cp -r vpr/SRC vtr_release/vpr
cp vpr/Makefile vtr_release/vpr
cp -r vpr/ARCH vtr_release/vpr/ARCH
cp vpr/VPR.vcxproj vtr_release/vpr
cp vpr/VPR.vcxproj.filters vtr_release/vpr
cp vpr/VPR.sln vtr_release/vpr
cp vpr/Readme.txt vtr_release/vpr
cp vpr/go.sh vtr_release/vpr
cp vpr/or1200.latren.blif vtr_release/vpr
cp vpr/sample_arch.xml vtr_release/vpr
cp vpr/VPR_User_Manual_7.0.pdf vtr_release/vpr

echo "Finishing Build"
cp README.release.txt vtr_release/README.txt
cp Makefile vtr_release/
cp -p run_reg_test.pl vtr_release/
cp -p run_quick_test.pl vtr_release/



echo "Remove .svn files"
rm -rf vtr_release/*/.svn
rm -rf vtr_release/*/*/.svn
rm -rf vtr_release/*/*/*/.svn
rm -rf vtr_release/*/*/*/*/.svn
rm -rf vtr_release/*/*/*/*/*/.svn
rm -rf vtr_release/*/*/*/*/*/*/.svn

echo "Create tarball"
tar -cf vtr_release.tar vtr_release
gzip vtr_release.tar

echo "Release complete"
