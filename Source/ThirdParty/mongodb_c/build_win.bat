setlocal

SET INSTALL_DIR="%cd%\Win64"

rd /s /q %INSTALL_DIR%
mkdir %INSTALL_DIR%

if not exist src\build_win mkdir src\build_win
pushd src\build_win
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% ..\mongo-c-driver && nmake install
popd
