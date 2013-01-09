cmake_minimum_required( VERSION 2.6 )
project( wsh )

option( BUILD_SERVER "Build wsh server" ON )
option( BUILD_CLIENT "Build wsh client" ON )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules )

set( CMAKE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )

set( WSH_INCLUDE_DIRS ${CMAKE_OUTPUT_DIRECTORY}/gen )
set( WSH_LIBRARIES wsh)

# Global deps
# https://raw.github.com/stevedekorte/io/master/modules/FindGLIB2.cmake
find_package( GLIB2 )
if( NOT GLIB2_FOUND )
	message( FATAL_ERROR "Could not find GLIB2" )
endif()

set( PROTOBUF_FIND_QUIETLY ON )
find_package( PROTOBUF-C )
if( NOT PROTOBUF_FOUND )
	message( FATAL_ERROR "Could not find protobuf-c" )
endif()

add_subdirectory( library/src )

if( BUILD_SERVER )
	add_subdirectory( server/src )
endif( BUILD_SERVER )

if( BUILD_CLIENT )
	add_subdirectory( client/src )
endif( BUILD_CLIENT )
