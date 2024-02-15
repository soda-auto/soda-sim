setlocal

SET INSTALL_DIR="%cd%\Win64"

SET MONGODB_C_DIR="%cd%\..\mongodb_c\Win64"
SET BOOST_DIR="C:\UE\UE_5.3\Engine\Source\ThirdParty\Boost\boost-1_80_0"

rd /s /q %INSTALL_DIR%
mkdir %INSTALL_DIR%

if not exist src\build_win mkdir src\build_win
pushd src\build_win
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DCMAKE_PREFIX_PATH=%MONGODB_C_DIR% -DBUILD_VERSION=3.7.0 -DBOOST_ROOT=%BOOST_DIR% -DBUILD_SHARED_AND_STATIC_LIBS=1 -DBUILD_SHARED_LIBS_WITH_STATIC_MONGOC=1 ..\mongo-cxx-driver && nmake install
popd
