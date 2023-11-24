@echo off

REM Mongo C++ Driver uninstall program, generated with CMake

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

pushd C:\UnrealProjects\ArrivalSim\Plugins\UnrealArrival\Source\ThirdParty\mongodb_cpp\Win64\

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
echo Removing file share\mongo-cxx-driver\LICENSE
del share\mongo-cxx-driver\LICENSE || echo ... not removed
echo Removing file share\mongo-cxx-driver\README.md
del share\mongo-cxx-driver\README.md || echo ... not removed
echo Removing file share\mongo-cxx-driver\THIRD-PARTY-NOTICES
del share\mongo-cxx-driver\THIRD-PARTY-NOTICES || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\array\element.hpp
del include\bsoncxx\v_noabi\bsoncxx\array\element.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\array\value.hpp
del include\bsoncxx\v_noabi\bsoncxx\array\value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\array\view.hpp
del include\bsoncxx\v_noabi\bsoncxx\array\view.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\array\view_or_value.hpp
del include\bsoncxx\v_noabi\bsoncxx\array\view_or_value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\basic\array.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\basic\array.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\basic\document.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\basic\document.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\basic\helpers.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\basic\helpers.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\basic\impl.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\basic\impl.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\basic\kvp.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\basic\kvp.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\basic\sub_array.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\basic\sub_array.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\basic\sub_document.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\basic\sub_document.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\concatenate.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\concatenate.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\core.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\core.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\list.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\list.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\stream\array.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\stream\array.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\stream\array_context.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\stream\array_context.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\stream\closed_context.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\stream\closed_context.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\stream\document.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\stream\document.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\stream\helpers.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\stream\helpers.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\stream\key_context.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\stream\key_context.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\stream\single_context.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\stream\single_context.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\builder\stream\value_context.hpp
del include\bsoncxx\v_noabi\bsoncxx\builder\stream\value_context.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\config\compiler.hpp
del include\bsoncxx\v_noabi\bsoncxx\config\compiler.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\config\postlude.hpp
del include\bsoncxx\v_noabi\bsoncxx\config\postlude.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\config\prelude.hpp
del include\bsoncxx\v_noabi\bsoncxx\config\prelude.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\decimal128.hpp
del include\bsoncxx\v_noabi\bsoncxx\decimal128.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\document\element.hpp
del include\bsoncxx\v_noabi\bsoncxx\document\element.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\document\value.hpp
del include\bsoncxx\v_noabi\bsoncxx\document\value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\document\view.hpp
del include\bsoncxx\v_noabi\bsoncxx\document\view.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\document\view_or_value.hpp
del include\bsoncxx\v_noabi\bsoncxx\document\view_or_value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\enums\binary_sub_type.hpp
del include\bsoncxx\v_noabi\bsoncxx\enums\binary_sub_type.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\enums\type.hpp
del include\bsoncxx\v_noabi\bsoncxx\enums\type.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\exception\error_code.hpp
del include\bsoncxx\v_noabi\bsoncxx\exception\error_code.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\exception\exception.hpp
del include\bsoncxx\v_noabi\bsoncxx\exception\exception.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\json.hpp
del include\bsoncxx\v_noabi\bsoncxx\json.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\oid.hpp
del include\bsoncxx\v_noabi\bsoncxx\oid.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\stdx\make_unique.hpp
del include\bsoncxx\v_noabi\bsoncxx\stdx\make_unique.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\stdx\optional.hpp
del include\bsoncxx\v_noabi\bsoncxx\stdx\optional.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\stdx\string_view.hpp
del include\bsoncxx\v_noabi\bsoncxx\stdx\string_view.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\string\to_string.hpp
del include\bsoncxx\v_noabi\bsoncxx\string\to_string.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\string\view_or_value.hpp
del include\bsoncxx\v_noabi\bsoncxx\string\view_or_value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\types\bson_value\make_value.hpp
del include\bsoncxx\v_noabi\bsoncxx\types\bson_value\make_value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\types\bson_value\value.hpp
del include\bsoncxx\v_noabi\bsoncxx\types\bson_value\value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\types\bson_value\view.hpp
del include\bsoncxx\v_noabi\bsoncxx\types\bson_value\view.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\types\bson_value\view_or_value.hpp
del include\bsoncxx\v_noabi\bsoncxx\types\bson_value\view_or_value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\types\value.hpp
del include\bsoncxx\v_noabi\bsoncxx\types\value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\types.hpp
del include\bsoncxx\v_noabi\bsoncxx\types.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\util\functor.hpp
del include\bsoncxx\v_noabi\bsoncxx\util\functor.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\validate.hpp
del include\bsoncxx\v_noabi\bsoncxx\validate.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\view_or_value.hpp
del include\bsoncxx\v_noabi\bsoncxx\view_or_value.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\config\export.hpp
del include\bsoncxx\v_noabi\bsoncxx\config\export.hpp || echo ... not removed
echo Removing file lib\cmake\libbsoncxx-3.7.0\libbsoncxx-config.cmake
del lib\cmake\libbsoncxx-3.7.0\libbsoncxx-config.cmake || echo ... not removed
echo Removing file lib\cmake\libbsoncxx-3.7.0\libbsoncxx-config-version.cmake
del lib\cmake\libbsoncxx-3.7.0\libbsoncxx-config-version.cmake || echo ... not removed
echo Removing file lib\cmake\libbsoncxx-static-3.7.0\libbsoncxx-static-config.cmake
del lib\cmake\libbsoncxx-static-3.7.0\libbsoncxx-static-config.cmake || echo ... not removed
echo Removing file lib\cmake\libbsoncxx-static-3.7.0\libbsoncxx-static-config-version.cmake
del lib\cmake\libbsoncxx-static-3.7.0\libbsoncxx-static-config-version.cmake || echo ... not removed
echo Removing file lib\bsoncxx.lib
del lib\bsoncxx.lib || echo ... not removed
echo Removing file bin\bsoncxx.dll
del bin\bsoncxx.dll || echo ... not removed
echo Removing file lib\bsoncxx-static.lib
del lib\bsoncxx-static.lib || echo ... not removed
echo Removing file lib\cmake\bsoncxx-3.7.0\bsoncxx_targets.cmake
del lib\cmake\bsoncxx-3.7.0\bsoncxx_targets.cmake || echo ... not removed
echo Removing file lib\cmake\bsoncxx-3.7.0\bsoncxx_targets-release.cmake
del lib\cmake\bsoncxx-3.7.0\bsoncxx_targets-release.cmake || echo ... not removed
echo Removing file lib\cmake\bsoncxx-3.7.0\bsoncxx-config-version.cmake
del lib\cmake\bsoncxx-3.7.0\bsoncxx-config-version.cmake || echo ... not removed
echo Removing file lib\cmake\bsoncxx-3.7.0\bsoncxx-config.cmake
del lib\cmake\bsoncxx-3.7.0\bsoncxx-config.cmake || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\config\config.hpp
del include\bsoncxx\v_noabi\bsoncxx\config\config.hpp || echo ... not removed
echo Removing file include\bsoncxx\v_noabi\bsoncxx\config\version.hpp
del include\bsoncxx\v_noabi\bsoncxx\config\version.hpp || echo ... not removed
echo Removing file lib\pkgconfig\libbsoncxx.pc
del lib\pkgconfig\libbsoncxx.pc || echo ... not removed
echo Removing file lib\pkgconfig\libbsoncxx-static.pc
del lib\pkgconfig\libbsoncxx-static.pc || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\bulk_write.hpp
del include\mongocxx\v_noabi\mongocxx\bulk_write.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\change_stream.hpp
del include\mongocxx\v_noabi\mongocxx\change_stream.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\client.hpp
del include\mongocxx\v_noabi\mongocxx\client.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\client_encryption.hpp
del include\mongocxx\v_noabi\mongocxx\client_encryption.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\client_session.hpp
del include\mongocxx\v_noabi\mongocxx\client_session.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\collection.hpp
del include\mongocxx\v_noabi\mongocxx\collection.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\config\compiler.hpp
del include\mongocxx\v_noabi\mongocxx\config\compiler.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\config\postlude.hpp
del include\mongocxx\v_noabi\mongocxx\config\postlude.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\config\prelude.hpp
del include\mongocxx\v_noabi\mongocxx\config\prelude.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\cursor.hpp
del include\mongocxx\v_noabi\mongocxx\cursor.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\database.hpp
del include\mongocxx\v_noabi\mongocxx\database.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\command_failed_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\command_failed_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\command_started_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\command_started_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\command_succeeded_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\command_succeeded_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\heartbeat_failed_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\heartbeat_failed_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\heartbeat_started_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\heartbeat_started_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\heartbeat_succeeded_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\heartbeat_succeeded_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\server_changed_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\server_changed_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\server_closed_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\server_closed_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\server_description.hpp
del include\mongocxx\v_noabi\mongocxx\events\server_description.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\server_opening_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\server_opening_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\topology_changed_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\topology_changed_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\topology_closed_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\topology_closed_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\topology_description.hpp
del include\mongocxx\v_noabi\mongocxx\events\topology_description.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\events\topology_opening_event.hpp
del include\mongocxx\v_noabi\mongocxx\events\topology_opening_event.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\authentication_exception.hpp
del include\mongocxx\v_noabi\mongocxx\exception\authentication_exception.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\bulk_write_exception.hpp
del include\mongocxx\v_noabi\mongocxx\exception\bulk_write_exception.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\error_code.hpp
del include\mongocxx\v_noabi\mongocxx\exception\error_code.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\exception.hpp
del include\mongocxx\v_noabi\mongocxx\exception\exception.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\gridfs_exception.hpp
del include\mongocxx\v_noabi\mongocxx\exception\gridfs_exception.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\logic_error.hpp
del include\mongocxx\v_noabi\mongocxx\exception\logic_error.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\operation_exception.hpp
del include\mongocxx\v_noabi\mongocxx\exception\operation_exception.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\query_exception.hpp
del include\mongocxx\v_noabi\mongocxx\exception\query_exception.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\server_error_code.hpp
del include\mongocxx\v_noabi\mongocxx\exception\server_error_code.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\exception\write_exception.hpp
del include\mongocxx\v_noabi\mongocxx\exception\write_exception.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\gridfs\bucket.hpp
del include\mongocxx\v_noabi\mongocxx\gridfs\bucket.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\gridfs\downloader.hpp
del include\mongocxx\v_noabi\mongocxx\gridfs\downloader.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\gridfs\uploader.hpp
del include\mongocxx\v_noabi\mongocxx\gridfs\uploader.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\hint.hpp
del include\mongocxx\v_noabi\mongocxx\hint.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\index_model.hpp
del include\mongocxx\v_noabi\mongocxx\index_model.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\index_view.hpp
del include\mongocxx\v_noabi\mongocxx\index_view.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\instance.hpp
del include\mongocxx\v_noabi\mongocxx\instance.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\logger.hpp
del include\mongocxx\v_noabi\mongocxx\logger.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\model\delete_many.hpp
del include\mongocxx\v_noabi\mongocxx\model\delete_many.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\model\delete_one.hpp
del include\mongocxx\v_noabi\mongocxx\model\delete_one.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\model\insert_one.hpp
del include\mongocxx\v_noabi\mongocxx\model\insert_one.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\model\replace_one.hpp
del include\mongocxx\v_noabi\mongocxx\model\replace_one.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\model\update_many.hpp
del include\mongocxx\v_noabi\mongocxx\model\update_many.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\model\update_one.hpp
del include\mongocxx\v_noabi\mongocxx\model\update_one.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\model\write.hpp
del include\mongocxx\v_noabi\mongocxx\model\write.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\aggregate.hpp
del include\mongocxx\v_noabi\mongocxx\options\aggregate.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\apm.hpp
del include\mongocxx\v_noabi\mongocxx\options\apm.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\auto_encryption.hpp
del include\mongocxx\v_noabi\mongocxx\options\auto_encryption.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\bulk_write.hpp
del include\mongocxx\v_noabi\mongocxx\options\bulk_write.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\change_stream.hpp
del include\mongocxx\v_noabi\mongocxx\options\change_stream.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\client.hpp
del include\mongocxx\v_noabi\mongocxx\options\client.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\client_encryption.hpp
del include\mongocxx\v_noabi\mongocxx\options\client_encryption.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\client_session.hpp
del include\mongocxx\v_noabi\mongocxx\options\client_session.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\count.hpp
del include\mongocxx\v_noabi\mongocxx\options\count.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\create_collection.hpp
del include\mongocxx\v_noabi\mongocxx\options\create_collection.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\data_key.hpp
del include\mongocxx\v_noabi\mongocxx\options\data_key.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\delete.hpp
del include\mongocxx\v_noabi\mongocxx\options\delete.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\distinct.hpp
del include\mongocxx\v_noabi\mongocxx\options\distinct.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\encrypt.hpp
del include\mongocxx\v_noabi\mongocxx\options\encrypt.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\estimated_document_count.hpp
del include\mongocxx\v_noabi\mongocxx\options\estimated_document_count.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\find.hpp
del include\mongocxx\v_noabi\mongocxx\options\find.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\find_one_and_delete.hpp
del include\mongocxx\v_noabi\mongocxx\options\find_one_and_delete.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\find_one_and_replace.hpp
del include\mongocxx\v_noabi\mongocxx\options\find_one_and_replace.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\find_one_and_update.hpp
del include\mongocxx\v_noabi\mongocxx\options\find_one_and_update.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\find_one_common_options.hpp
del include\mongocxx\v_noabi\mongocxx\options\find_one_common_options.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\gridfs\bucket.hpp
del include\mongocxx\v_noabi\mongocxx\options\gridfs\bucket.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\gridfs\upload.hpp
del include\mongocxx\v_noabi\mongocxx\options\gridfs\upload.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\index.hpp
del include\mongocxx\v_noabi\mongocxx\options\index.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\index_view.hpp
del include\mongocxx\v_noabi\mongocxx\options\index_view.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\insert.hpp
del include\mongocxx\v_noabi\mongocxx\options\insert.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\pool.hpp
del include\mongocxx\v_noabi\mongocxx\options\pool.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\replace.hpp
del include\mongocxx\v_noabi\mongocxx\options\replace.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\server_api.hpp
del include\mongocxx\v_noabi\mongocxx\options\server_api.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\ssl.hpp
del include\mongocxx\v_noabi\mongocxx\options\ssl.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\tls.hpp
del include\mongocxx\v_noabi\mongocxx\options\tls.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\transaction.hpp
del include\mongocxx\v_noabi\mongocxx\options\transaction.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\options\update.hpp
del include\mongocxx\v_noabi\mongocxx\options\update.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\pipeline.hpp
del include\mongocxx\v_noabi\mongocxx\pipeline.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\pool.hpp
del include\mongocxx\v_noabi\mongocxx\pool.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\read_concern.hpp
del include\mongocxx\v_noabi\mongocxx\read_concern.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\read_preference.hpp
del include\mongocxx\v_noabi\mongocxx\read_preference.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\result\bulk_write.hpp
del include\mongocxx\v_noabi\mongocxx\result\bulk_write.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\result\delete.hpp
del include\mongocxx\v_noabi\mongocxx\result\delete.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\result\gridfs\upload.hpp
del include\mongocxx\v_noabi\mongocxx\result\gridfs\upload.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\result\insert_many.hpp
del include\mongocxx\v_noabi\mongocxx\result\insert_many.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\result\insert_one.hpp
del include\mongocxx\v_noabi\mongocxx\result\insert_one.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\result\replace_one.hpp
del include\mongocxx\v_noabi\mongocxx\result\replace_one.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\result\update.hpp
del include\mongocxx\v_noabi\mongocxx\result\update.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\stdx.hpp
del include\mongocxx\v_noabi\mongocxx\stdx.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\uri.hpp
del include\mongocxx\v_noabi\mongocxx\uri.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\validation_criteria.hpp
del include\mongocxx\v_noabi\mongocxx\validation_criteria.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\write_concern.hpp
del include\mongocxx\v_noabi\mongocxx\write_concern.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\write_type.hpp
del include\mongocxx\v_noabi\mongocxx\write_type.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\config\export.hpp
del include\mongocxx\v_noabi\mongocxx\config\export.hpp || echo ... not removed
echo Removing file lib\cmake\libmongocxx-3.7.0\libmongocxx-config.cmake
del lib\cmake\libmongocxx-3.7.0\libmongocxx-config.cmake || echo ... not removed
echo Removing file lib\cmake\libmongocxx-3.7.0\libmongocxx-config-version.cmake
del lib\cmake\libmongocxx-3.7.0\libmongocxx-config-version.cmake || echo ... not removed
echo Removing file lib\cmake\libmongocxx-static-3.7.0\libmongocxx-static-config.cmake
del lib\cmake\libmongocxx-static-3.7.0\libmongocxx-static-config.cmake || echo ... not removed
echo Removing file lib\cmake\libmongocxx-static-3.7.0\libmongocxx-static-config-version.cmake
del lib\cmake\libmongocxx-static-3.7.0\libmongocxx-static-config-version.cmake || echo ... not removed
echo Removing file lib\mongocxx.lib
del lib\mongocxx.lib || echo ... not removed
echo Removing file bin\mongocxx.dll
del bin\mongocxx.dll || echo ... not removed
echo Removing file lib\mongocxx-static.lib
del lib\mongocxx-static.lib || echo ... not removed
echo Removing file lib\cmake\mongocxx-3.7.0\mongocxx_targets.cmake
del lib\cmake\mongocxx-3.7.0\mongocxx_targets.cmake || echo ... not removed
echo Removing file lib\cmake\mongocxx-3.7.0\mongocxx_targets-release.cmake
del lib\cmake\mongocxx-3.7.0\mongocxx_targets-release.cmake || echo ... not removed
echo Removing file lib\cmake\mongocxx-3.7.0\mongocxx-config-version.cmake
del lib\cmake\mongocxx-3.7.0\mongocxx-config-version.cmake || echo ... not removed
echo Removing file lib\cmake\mongocxx-3.7.0\mongocxx-config.cmake
del lib\cmake\mongocxx-3.7.0\mongocxx-config.cmake || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\config\config.hpp
del include\mongocxx\v_noabi\mongocxx\config\config.hpp || echo ... not removed
echo Removing file include\mongocxx\v_noabi\mongocxx\config\version.hpp
del include\mongocxx\v_noabi\mongocxx\config\version.hpp || echo ... not removed
echo Removing file lib\pkgconfig\libmongocxx.pc
del lib\pkgconfig\libmongocxx.pc || echo ... not removed
echo Removing file lib\pkgconfig\libmongocxx-static.pc
del lib\pkgconfig\libmongocxx-static.pc || echo ... not removed
echo Removing file share\mongo-cxx-driver\uninstall.cmd
del share\mongo-cxx-driver\uninstall.cmd || echo ... not removed
echo Removing directory bin
(rmdir bin 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\array
(rmdir include\bsoncxx\v_noabi\bsoncxx\array 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\builder\basic
(rmdir include\bsoncxx\v_noabi\bsoncxx\builder\basic 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\builder\stream
(rmdir include\bsoncxx\v_noabi\bsoncxx\builder\stream 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\builder
(rmdir include\bsoncxx\v_noabi\bsoncxx\builder 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\config
(rmdir include\bsoncxx\v_noabi\bsoncxx\config 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\document
(rmdir include\bsoncxx\v_noabi\bsoncxx\document 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\enums
(rmdir include\bsoncxx\v_noabi\bsoncxx\enums 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\exception
(rmdir include\bsoncxx\v_noabi\bsoncxx\exception 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\stdx
(rmdir include\bsoncxx\v_noabi\bsoncxx\stdx 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\string
(rmdir include\bsoncxx\v_noabi\bsoncxx\string 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\types\bson_value
(rmdir include\bsoncxx\v_noabi\bsoncxx\types\bson_value 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\types
(rmdir include\bsoncxx\v_noabi\bsoncxx\types 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx\util
(rmdir include\bsoncxx\v_noabi\bsoncxx\util 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi\bsoncxx
(rmdir include\bsoncxx\v_noabi\bsoncxx 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx\v_noabi
(rmdir include\bsoncxx\v_noabi 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\bsoncxx
(rmdir include\bsoncxx 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx\config
(rmdir include\mongocxx\v_noabi\mongocxx\config 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx\events
(rmdir include\mongocxx\v_noabi\mongocxx\events 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx\exception
(rmdir include\mongocxx\v_noabi\mongocxx\exception 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx\gridfs
(rmdir include\mongocxx\v_noabi\mongocxx\gridfs 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx\model
(rmdir include\mongocxx\v_noabi\mongocxx\model 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx\options\gridfs
(rmdir include\mongocxx\v_noabi\mongocxx\options\gridfs 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx\options
(rmdir include\mongocxx\v_noabi\mongocxx\options 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx\result\gridfs
(rmdir include\mongocxx\v_noabi\mongocxx\result\gridfs 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx\result
(rmdir include\mongocxx\v_noabi\mongocxx\result 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi\mongocxx
(rmdir include\mongocxx\v_noabi\mongocxx 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx\v_noabi
(rmdir include\mongocxx\v_noabi 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include\mongocxx
(rmdir include\mongocxx 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory include
(rmdir include 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\bsoncxx-3.7.0
(rmdir lib\cmake\bsoncxx-3.7.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\libbsoncxx-3.7.0
(rmdir lib\cmake\libbsoncxx-3.7.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\libbsoncxx-static-3.7.0
(rmdir lib\cmake\libbsoncxx-static-3.7.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\libmongocxx-3.7.0
(rmdir lib\cmake\libmongocxx-3.7.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\libmongocxx-static-3.7.0
(rmdir lib\cmake\libmongocxx-static-3.7.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake\mongocxx-3.7.0
(rmdir lib\cmake\mongocxx-3.7.0 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\cmake
(rmdir lib\cmake 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib\pkgconfig
(rmdir lib\pkgconfig 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory lib
(rmdir lib 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory share\mongo-cxx-driver
(rmdir share\mongo-cxx-driver 2>NUL) || echo ... not removed (probably not empty)
echo Removing directory share
(rmdir share 2>NUL) || echo ... not removed (probably not empty)
cd ..
echo Removing top-level installation directory: C:\UnrealProjects\ArrivalSim\Plugins\UnrealArrival\Source\ThirdParty\mongodb_cpp\Win64\
(rmdir C:\UnrealProjects\ArrivalSim\Plugins\UnrealArrival\Source\ThirdParty\mongodb_cpp\Win64\ 2>NUL) || echo ... not removed (probably not empty)

REM Return to the directory from which the program was called
popd
