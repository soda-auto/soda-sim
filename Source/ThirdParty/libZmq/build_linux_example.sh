UE_LIBCXX=$UE4_ROOT/Engine/Source/ThirdParty/Linux/LibCxx
UE_SYSROOT=$UE4_ROOT/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v19_clang-11.0.1-centos7/x86_64-unknown-linux-gnu
INSTALL_DIR=$PWD/Linux

CC=$UE_SYSROOT/bin/clang
CXX=$UE_SYSROOT/bin/clang++
CCFLAGS="-nostdinc++ -nostdlib++ -g3 -O3 -I$UE_SYSROOT/usr/include -I$UE_LIBCXX/include -I$UE_LIBCXX/include/c++/v1 --sysroot=$UE_SYSROOT"
CXXFLAGS=" -nostdinc++ -nostdlib++ -g3 -O3 -std=c++11 -I$UE_SYSROOT/usr/include -I$UE_LIBCXX/include -I$UE_LIBCXX/include/c++/v1 --sysroot=$UE_SYSROOT"
LDFLAGS="-nodefaultlibs -stdlib=libc++  --gcc-toolchain=$UE_SYSROOT --sysroot=$UE_SYSROOT -B$UE_SYSROOT/usr/lib64 -L$UE_SYSROOT/usr/lib64 -L$UE_LIBCXX/lib/Linux/x86_64-unknown-linux-gnu/" 
LIBS="$UE_LIBCXX/lib/Linux/x86_64-unknown-linux-gnu/libc++.a $UE_LIBCXX/lib/Linux/x86_64-unknown-linux-gnu/libc++abi.a -lc -lm -lgcc_s -lgcc"


mkdir -p $INSTALL_DIR && rm -rf $INSTALL_DIR/*
rm -rf build &&  mkdir build && pushd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      -DCMAKE_C_COMPILER="$CC" \
      -DCMAKE_CXX_COMPILER="$CXX" \
      -DCMAKE_MODULE_LINKER_FLAGS="$LIBS" \
      -DCMAKE_EXE_LINKER_FLAGS="$LIBS" \
      -DCMAKE_SHARED_LINKER_FLAGS="$LIBS" \
      -DCMAKE_C_FLAGS="$CCFLAGS" \
      -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
      -DCMAKE_CXX_STANDARD_LIBRARIES="$LIBS" \
      -DBUILD_SHARED=ON \
      -DBUILD_STATIC=OFF \
      -DBUILD_TESTS=OFF \
      -DWITH_LIBSODIUM=OFF .. && make -j8 install
popd


