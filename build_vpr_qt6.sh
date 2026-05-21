mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=release -G 'Unix Makefiles' -DCMAKE_COMPILE_WARNING_AS_ERROR=on -DVTR_IPO_BUILD=off -DVTR_ASSERT_LEVEL=3 -DCMAKE_PREFIX_PATH=/opt/qt6/6.9.3/gcc_64 ..
make -j$(nproc)


