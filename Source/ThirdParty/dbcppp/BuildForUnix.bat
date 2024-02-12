
@echo off

set ENGINE_ROOT=C:\UE\UnrealEngine-5.3.2-release\Engine
set THIRDPARTY_ROOT=C:\UnrealProjects\SodaSimProject\Plugins\SodaSim\Source\ThirdParty

set CMAKE_ADDITIONAL_ARGUMENTS=-DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH=OFF -DBUILD_WITH_LIBCXX=ON

call "%ENGINE_ROOT%\Build\BatchFiles\RunUAT.bat" BuildCMakeLib -TargetPlatform=Unix -TargetArchitecture=x86_64-unknown-linux-gnu -TargetLib=dbcppp -TargetConfigs=Release -LibOutputPath=lib -CMakeGenerator=Makefile -CMakeAdditionalArguments="%CMAKE_ADDITIONAL_ARGUMENTS%" -SkipCreateChangelist || exit /b
