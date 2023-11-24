setlocal

SET TOOLCHAIN_FILE="%cd%\..\unreal-linux-from-windows-toolchain.cmake"
SET MAKE_EXE="C:\Program Files (x86)\GnuWin32\bin\make.exe"
SET MONGODB_C_DIR="%cd%\..\mongodb_c\Linux"
SET BOOST_DIR="C:\UE\UE_5.1\Engine\Source\ThirdParty\Boost\boost-1_70_0" 
SET INSTALL_DIR="%cd%\Linux"

rd \s \q %INSTALL_DIR%
mkdir %INSTALL_DIR%

if not exist src\build_linux mkdir src\build_linux
pushd src\build_linux

cmake -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN_FILE% -DCMAKE_MAKE_PROGRAM=%MAKE_EXE% -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DCMAKE_PREFIX_PATH=%MONGODB_C_DIR% -DBUILD_VERSION=3.7.0 -DBOOST_ROOT=%BOOST_DIR% -DBSONCXX_POLY_USE_BOOST=ON -DENABLE_TESTS=OFF -DBUILD_SHARED_AND_STATIC_LIBS=1 .. && make -j8 install

popd
