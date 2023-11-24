@echo off

REM Mongo C Driver uninstall program, generated with CMake

REM Copyright 2018-present MongoDB, Inc.
REM
REM Licensed under the Apache License, Version 2.0 (the "License");
REM you may not use this file except in compliance with the License.
REM You may obtain a copy of the License at
REM
REM   http://www.apache.org/licenses/LICENSE-2.0
REM
REM Unless required by applicable law or agreed to in writing, software
REM distributed under the License is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM See the License for the specific language governing permissions and
REM limitations under the License.



REM Windows does not handle a batch script deleting itself during
REM execution.  Copy the uninstall program into the TEMP directory from
REM the environment and run from there so everything in the installation
REM is deleted and the copied program deletes itself at the end.
if /i "%~dp0" NEQ "%TEMP%\" (
   copy "%~f0" "%TEMP%\mongoc-%~nx0" >NUL
   "%TEMP%\mongoc-%~nx0" & del "%TEMP%\mongoc-%~nx0"
)

pushd C:\UnrealProjects\ArrivalSim\Plugins\UnrealArrival\Source\ThirdParty\mongodb_c\Win64\

echo Removing file bin\msvcp140.dll
del bin\msvcp140.dll || echo ... not removed
echo Removing file bin\msvcp140_1.dll
del bin\msvcp140_1.dll || echo ... not removed
echo Removing file bin\msvcp140_2.dll
del bin\msvcp140_2.dll || echo ... not removed
echo Removing file bin\msvcp140_atomic_wait.dll
del bin\msvcp140_atomic_wait.dll || echo ... not removed
echo Removing file bin\msvcp140_codecvt_ids.dll
del bin\msvcp140_codecvt_ids.dll || echo ... not removed
echo Removing file bin\vcruntime140_1.dll
del bin\vcruntime140_1.dll || echo ... not removed
echo Removing file bin\vcruntime140.dll
del bin\vcruntime140.dll || echo ... not removed
echo Removing file bin\concrt140.dll
del bin\concrt140.dll || echo ... not removed
echo Removing file share\mongo-c-driver\COPYING
del share\mongo-c-driver\COPYING || echo ... not removed
echo Removing file share\mongo-c-driver\NEWS
del share\mongo-c-driver\NEWS || echo ... not removed
echo Removing file share\mongo-c-driver\README.rst
del share\mongo-c-driver\README.rst || echo ... not removed
echo Removing file share\mongo-c-driver\THIRD_PARTY_NOTICES
del share\mongo-c-driver\THIRD_PARTY_NOTICES || echo ... not removed
echo Removing file bin\msvcp140.dll
del bin\msvcp140.dll || echo ... not removed
echo Removing file bin\msvcp140_1.dll
del bin\msvcp140_1.dll || echo ... not removed
echo Removing file bin\msvcp140_2.dll
del bin\msvcp140_2.dll || echo ... not removed
echo Removing file bin\msvcp140_atomic_wait.dll
del bin\msvcp140_atomic_wait.dll || echo ... not removed
echo Removing file bin\msvcp140_codecvt_ids.dll
del bin\msvcp140_codecvt_ids.dll || echo ... not removed
echo Removing file bin\vcruntime140_1.dll
del bin\vcruntime140_1.dll || echo ... not removed
echo Removing file bin\vcruntime140.dll
del bin\vcruntime140.dll || echo ... not removed
echo Removing file bin\concrt140.dll
del bin\concrt140.dll || echo ... not removed
echo Removing file bin\msvcp140.dll
del bin\msvcp140.dll || echo ... not removed
echo Removing file bin\msvcp140_1.dll
del bin\msvcp140_1.dll || echo ... not removed
echo Removing file bin\msvcp140_2.dll
del bin\msvcp140_2.dll || echo ... not removed
echo Removing file bin\msvcp140_atomic_wait.dll
del bin\msvcp140_atomic_wait.dll || echo ... not removed
echo Removing file bin\msvcp140_codecvt_ids.dll
del bin\msvcp140_codecvt_ids.dll || echo ... not removed
echo Removing file bin\vcruntime140_1.dll
del bin\vcruntime140_1.dll || echo ... not removed
echo Removing file bin\vcruntime140.dll
del bin\vcruntime140.dll || echo ... not removed
echo Removing file bin\concrt140.dll
del bin\concrt140.dll || echo ... not removed
echo Removing file lib\bson-1.0.lib
del lib\bson-1.0.lib || echo ... not removed
echo Removing file bin\bson-1.0.dll
del bin\bson-1.0.dll || echo ... not removed
echo Removing file lib\bson-static-1.0.lib
del lib\bson-static-1.0.lib || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-config.h
del include\libbson-1.0\bson\bson-config.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-version.h
del include\libbson-1.0\bson\bson-version.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bcon.h
del include\libbson-1.0\bson\bcon.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-atomic.h
del include\libbson-1.0\bson\bson-atomic.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-clock.h
del include\libbson-1.0\bson\bson-clock.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-cmp.h
del include\libbson-1.0\bson\bson-cmp.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-compat.h
del include\libbson-1.0\bson\bson-compat.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-context.h
del include\libbson-1.0\bson\bson-context.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-decimal128.h
del include\libbson-1.0\bson\bson-decimal128.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-endian.h
del include\libbson-1.0\bson\bson-endian.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-error.h
del include\libbson-1.0\bson\bson-error.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson.h
del include\libbson-1.0\bson\bson.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-iter.h
del include\libbson-1.0\bson\bson-iter.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-json.h
del include\libbson-1.0\bson\bson-json.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-keys.h
del include\libbson-1.0\bson\bson-keys.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-macros.h
del include\libbson-1.0\bson\bson-macros.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-md5.h
del include\libbson-1.0\bson\bson-md5.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-memory.h
del include\libbson-1.0\bson\bson-memory.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-oid.h
del include\libbson-1.0\bson\bson-oid.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-prelude.h
del include\libbson-1.0\bson\bson-prelude.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-reader.h
del include\libbson-1.0\bson\bson-reader.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-string.h
del include\libbson-1.0\bson\bson-string.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-types.h
del include\libbson-1.0\bson\bson-types.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-utf8.h
del include\libbson-1.0\bson\bson-utf8.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-value.h
del include\libbson-1.0\bson\bson-value.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-version-functions.h
del include\libbson-1.0\bson\bson-version-functions.h || echo ... not removed
echo Removing file include\libbson-1.0\bson\bson-writer.h
del include\libbson-1.0\bson\bson-writer.h || echo ... not removed
echo Removing file include\libbson-1.0\bson.h
del include\libbson-1.0\bson.h || echo ... not removed
echo Removing file lib\pkgconfig\libbson-1.0.pc
del lib\pkgconfig\libbson-1.0.pc || echo ... not removed
echo Removing file lib\pkgconfig\libbson-static-1.0.pc
del lib\pkgconfig\libbson-static-1.0.pc || echo ... not removed
echo Removing file lib\cmake\bson-1.0\bson-targets.cmake
del lib\cmake\bson-1.0\bson-targets.cmake || echo ... not removed
echo Removing file lib\cmake\bson-1.0\bson-targets-release.cmake
del lib\cmake\bson-1.0\bson-targets-release.cmake || echo ... not removed
echo Removing file lib\cmake\bson-1.0\bson-1.0-config.cmake
del lib\cmake\bson-1.0\bson-1.0-config.cmake || echo ... not removed
echo Removing file lib\cmake\bson-1.0\bson-1.0-config-version.cmake
del lib\cmake\bson-1.0\bson-1.0-config-version.cmake || echo ... not removed
echo Removing file lib\cmake\libbson-1.0\libbson-1.0-config.cmake
del lib\cmake\libbson-1.0\libbson-1.0-config.cmake || echo ... not removed
echo Removing file lib\cmake\libbson-1.0\libbson-1.0-config-version.cmake
del lib\cmake\libbson-1.0\libbson-1.0-config-version.cmake || echo ... not removed
echo Removing file lib\cmake\libbson-static-1.0\libbson-static-1.0-config.cmake
del lib\cmake\libbson-static-1.0\libbson-static-1.0-config.cmake || echo ... not removed
echo Removing file lib\cmake\libbson-static-1.0\libbson-static-1.0-config-version.cmake
del lib\cmake\libbson-static-1.0\libbson-static-1.0-config-version.cmake || echo ... not removed
echo Removing file bin\msvcp140.dll
del bin\msvcp140.dll || echo ... not removed
echo Removing file bin\msvcp140_1.dll
del bin\msvcp140_1.dll || echo ... not removed
echo Removing file bin\msvcp140_2.dll
del bin\msvcp140_2.dll || echo ... not removed
echo Removing file bin\msvcp140_atomic_wait.dll
del bin\msvcp140_atomic_wait.dll || echo ... not removed
echo Removing file bin\msvcp140_codecvt_ids.dll
del bin\msvcp140_codecvt_ids.dll || echo ... not removed
echo Removing file bin\vcruntime140_1.dll
del bin\vcruntime140_1.dll || echo ... not removed
echo Removing file bin\vcruntime140.dll
del bin\vcruntime140.dll || echo ... not removed
echo Removing file bin\concrt140.dll
del bin\concrt140.dll || echo ... not removed
echo Removing file bin\msvcp140.dll
del bin\msvcp140.dll || echo ... not removed
echo Removing file bin\msvcp140_1.dll
del bin\msvcp140_1.dll || echo ... not removed
echo Removing file bin\msvcp140_2.dll
del bin\msvcp140_2.dll || echo ... not removed
echo Removing file bin\msvcp140_atomic_wait.dll
del bin\msvcp140_atomic_wait.dll || echo ... not removed
echo Removing file bin\msvcp140_codecvt_ids.dll
del bin\msvcp140_codecvt_ids.dll || echo ... not removed
echo Removing file bin\vcruntime140_1.dll
del bin\vcruntime140_1.dll || echo ... not removed
echo Removing file bin\vcruntime140.dll
del bin\vcruntime140.dll || echo ... not removed
echo Removing file bin\concrt140.dll
del bin\concrt140.dll || echo ... not removed
echo Removing file lib\mongoc-1.0.lib
del lib\mongoc-1.0.lib || echo ... not removed
echo Removing file bin\mongoc-1.0.dll
del bin\mongoc-1.0.dll || echo ... not removed
echo Removing file lib\mongoc-static-1.0.lib
del lib\mongoc-static-1.0.lib || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-config.h
del include\libmongoc-1.0\mongoc\mongoc-config.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-version.h
del include\libmongoc-1.0\mongoc\mongoc-version.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc.h
del include\libmongoc-1.0\mongoc\mongoc.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-apm.h
del include\libmongoc-1.0\mongoc\mongoc-apm.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-bulk-operation.h
del include\libmongoc-1.0\mongoc\mongoc-bulk-operation.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-change-stream.h
del include\libmongoc-1.0\mongoc\mongoc-change-stream.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-client.h
del include\libmongoc-1.0\mongoc\mongoc-client.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-client-pool.h
del include\libmongoc-1.0\mongoc\mongoc-client-pool.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-client-side-encryption.h
del include\libmongoc-1.0\mongoc\mongoc-client-side-encryption.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-collection.h
del include\libmongoc-1.0\mongoc\mongoc-collection.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-cursor.h
del include\libmongoc-1.0\mongoc\mongoc-cursor.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-database.h
del include\libmongoc-1.0\mongoc\mongoc-database.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-error.h
del include\libmongoc-1.0\mongoc\mongoc-error.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-flags.h
del include\libmongoc-1.0\mongoc\mongoc-flags.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-find-and-modify.h
del include\libmongoc-1.0\mongoc\mongoc-find-and-modify.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-gridfs.h
del include\libmongoc-1.0\mongoc\mongoc-gridfs.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-gridfs-bucket.h
del include\libmongoc-1.0\mongoc\mongoc-gridfs-bucket.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-gridfs-file.h
del include\libmongoc-1.0\mongoc\mongoc-gridfs-file.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-gridfs-file-page.h
del include\libmongoc-1.0\mongoc\mongoc-gridfs-file-page.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-gridfs-file-list.h
del include\libmongoc-1.0\mongoc\mongoc-gridfs-file-list.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-handshake.h
del include\libmongoc-1.0\mongoc\mongoc-handshake.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-host-list.h
del include\libmongoc-1.0\mongoc\mongoc-host-list.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-init.h
del include\libmongoc-1.0\mongoc\mongoc-init.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-index.h
del include\libmongoc-1.0\mongoc\mongoc-index.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-iovec.h
del include\libmongoc-1.0\mongoc\mongoc-iovec.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-log.h
del include\libmongoc-1.0\mongoc\mongoc-log.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-macros.h
del include\libmongoc-1.0\mongoc\mongoc-macros.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-matcher.h
del include\libmongoc-1.0\mongoc\mongoc-matcher.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-opcode.h
del include\libmongoc-1.0\mongoc\mongoc-opcode.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-optional.h
del include\libmongoc-1.0\mongoc\mongoc-optional.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-prelude.h
del include\libmongoc-1.0\mongoc\mongoc-prelude.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-read-concern.h
del include\libmongoc-1.0\mongoc\mongoc-read-concern.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-read-prefs.h
del include\libmongoc-1.0\mongoc\mongoc-read-prefs.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-server-api.h
del include\libmongoc-1.0\mongoc\mongoc-server-api.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-server-description.h
del include\libmongoc-1.0\mongoc\mongoc-server-description.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-client-session.h
del include\libmongoc-1.0\mongoc\mongoc-client-session.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-socket.h
del include\libmongoc-1.0\mongoc\mongoc-socket.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-stream-tls-libressl.h
del include\libmongoc-1.0\mongoc\mongoc-stream-tls-libressl.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-stream-tls-openssl.h
del include\libmongoc-1.0\mongoc\mongoc-stream-tls-openssl.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-stream.h
del include\libmongoc-1.0\mongoc\mongoc-stream.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-stream-buffered.h
del include\libmongoc-1.0\mongoc\mongoc-stream-buffered.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-stream-file.h
del include\libmongoc-1.0\mongoc\mongoc-stream-file.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-stream-gridfs.h
del include\libmongoc-1.0\mongoc\mongoc-stream-gridfs.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-stream-socket.h
del include\libmongoc-1.0\mongoc\mongoc-stream-socket.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-topology-description.h
del include\libmongoc-1.0\mongoc\mongoc-topology-description.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-uri.h
del include\libmongoc-1.0\mongoc\mongoc-uri.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-version-functions.h
del include\libmongoc-1.0\mongoc\mongoc-version-functions.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-write-concern.h
del include\libmongoc-1.0\mongoc\mongoc-write-concern.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-rand.h
del include\libmongoc-1.0\mongoc\mongoc-rand.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-stream-tls.h
del include\libmongoc-1.0\mongoc\mongoc-stream-tls.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc\mongoc-ssl.h
del include\libmongoc-1.0\mongoc\mongoc-ssl.h || echo ... not removed
echo Removing file include\libmongoc-1.0\mongoc.h
del include\libmongoc-1.0\mongoc.h || echo ... not removed
echo Removing file lib\pkgconfig\libmongoc-1.0.pc
del lib\pkgconfig\libmongoc-1.0.pc || echo ... not removed
echo Removing file lib\pkgconfig\libmongoc-static-1.0.pc
del lib\pkgconfig\libmongoc-static-1.0.pc || echo ... not removed
echo Removing file lib\pkgconfig\libmongoc-ssl-1.0.pc
del lib\pkgconfig\libmongoc-ssl-1.0.pc || echo ... not removed
echo Removing file lib\cmake\mongoc-1.0\mongoc-targets.cmake
del lib\cmake\mongoc-1.0\mongoc-targets.cmake || echo ... not removed
echo Removing file lib\cmake\mongoc-1.0\mongoc-targets-release.cmake
del lib\cmake\mongoc-1.0\mongoc-targets-release.cmake || echo ... not removed
echo Removing file lib\cmake\mongoc-1.0\mongoc-1.0-config.cmake
del lib\cmake\mongoc-1.0\mongoc-1.0-config.cmake || echo ... not removed
echo Removing file lib\cmake\mongoc-1.0\mongoc-1.0-config-version.cmake
del lib\cmake\mongoc-1.0\mongoc-1.0-config-version.cmake || echo ... not removed
echo Removing file lib\cmake\libmongoc-1.0\libmongoc-1.0-config.cmake
del lib\cmake\libmongoc-1.0\libmongoc-1.0-config.cmake || echo ... not removed
echo Removing file lib\cmake\libmongoc-1.0\libmongoc-1.0-config-version.cmake
del lib\cmake\libmongoc-1.0\libmongoc-1.0-config-version.cmake || echo ... not removed
echo Removing file lib\cmake\libmongoc-static-1.0\libmongoc-static-1.0-config.cmake
del lib\cmake\libmongoc-static-1.0\libmongoc-static-1.0-config.cmake || echo ... not removed
echo Removing file lib\cmake\libmongoc-static-1.0\libmongoc-static-1.0-config-version.cmake
del lib\cmake\libmongoc-static-1.0\libmongoc-static-1.0-config-version.cmake || echo ... not removed
echo Removing file share\mongo-c-driver\uninstall.cmd
del share\mongo-c-driver\uninstall.cmd || echo ... not removed
echo Removing directory bin
(rmdir bin 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\libbson-1.0\bson
(rmdir include\libbson-1.0\bson 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\libbson-1.0
(rmdir include\libbson-1.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\libmongoc-1.0\mongoc
(rmdir include\libmongoc-1.0\mongoc 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\libmongoc-1.0
(rmdir include\libmongoc-1.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include
(rmdir include 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\bson-1.0
(rmdir lib\cmake\bson-1.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\libbson-1.0
(rmdir lib\cmake\libbson-1.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\libbson-static-1.0
(rmdir lib\cmake\libbson-static-1.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\libmongoc-1.0
(rmdir lib\cmake\libmongoc-1.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\libmongoc-static-1.0
(rmdir lib\cmake\libmongoc-static-1.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\mongoc-1.0
(rmdir lib\cmake\mongoc-1.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake
(rmdir lib\cmake 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\pkgconfig
(rmdir lib\pkgconfig 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib
(rmdir lib 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory share\mongo-c-driver
(rmdir share\mongo-c-driver 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory share
(rmdir share 2>NUL) || echo ... not removed (probably not empty)
cd ..
echo Removing top-level installation directory: C:\UnrealProjects\ArrivalSim\Plugins\UnrealArrival\Source\ThirdParty\mongodb_c\Win64\
(rmdir C:\UnrealProjects\ArrivalSim\Plugins\UnrealArrival\Source\ThirdParty\mongodb_c\Win64\ 2>NUL) || echo ... not removed (probably not empty)

REM Return to the directory from which the program was called
popd
