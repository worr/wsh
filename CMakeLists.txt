cmake_minimum_required( VERSION 2.6 )
project( wsh )

option( BUILD_SERVER "Build wsh server" ON )
option( BUILD_CLIENT "Build wsh client" ON )
option( BUILD_TESTS  "Build wsh unit tests" OFF )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules )

set( CMAKE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )

set( WSH_INCLUDE_DIRS ${CMAKE_OUTPUT_DIRECTORY}/gen ${CMAKE_SOURCE_DIR}/library/src )
set( WSH_LIBRARIES wsh)

add_definitions( -std=c99 )
add_definitions( -Wall -Werror -pedantic )

# Global deps
# https://raw.github.com/stevedekorte/io/master/modules/FindGLIB2.cmake
find_package( GLIB2 REQUIRED )

# https://github.com/Pronghorn/pronghorn/blob/master/FindPROTOBUF-C.cmake
find_package( PROTOBUF-C REQUIRED )

find_package( Threads REQUIRED )

# This is a hack to get around problems with the custom cmake libs I found
if( IS_DIRECTORY /usr/local/lib )
	link_directories( /usr/local/lib )
elseif( IS_DIRECTORY /usr/pkg/lib )
	link_directories( /usr/pkg/lib )
endif( IS_DIRECTORY /usr/local/lib )

add_subdirectory( library/src )

if( BUILD_TESTS )
	# enable testing for whole project
	enable_testing()

	add_subdirectory( library/test )
endif( BUILD_TESTS )

if( BUILD_SERVER )
	add_subdirectory( server/src )
	if( BUILD_TESTS )
		add_subdirectory( server/test )
	endif( BUILD_TESTS )
endif( BUILD_SERVER )

if( BUILD_CLIENT )
	add_subdirectory( client/src )
	if( BUILD_TESTS )
		#add_subdirectory( client/test )
	endif( BUILD_TESTS )
endif( BUILD_CLIENT )
