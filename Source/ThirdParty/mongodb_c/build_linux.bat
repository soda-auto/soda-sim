setlocal

SET TOOLCHAIN_FILE="%cd%\..\unreal-linux-from-windows-toolchain.cmake"
SET MAKE_EXE="C:\Program Files (x86)\GnuWin32\bin\make.exe" 
SET INSTALL_DIR="%cd%\Linux"

rd \s \q %INSTALL_DIR%
mkdir %INSTALL_DIR%

if not exist src\build_linux mkdir src\build_linux
pushd src\build_linux

cmake -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN_FILE% -DCMAKE_MAKE_PROGRAM=%MAKE_EXE% -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DENABLE_MONGODB_AWS_AUTH=OFF -DMONGOC_TEST_USE_CRYPT_SHARED=OFF ..\mongo-c-driver && make -j8 install

popd
