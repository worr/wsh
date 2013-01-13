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

set( WSH_INCLUDE_DIRS ${CMAKE_OUTPUT_DIRECTORY}/gen ${CMAKE_OUTPUT_DIRECTORY}/include )
set( WSH_LIBRARIES wsh)

add_definitions( -std=c99 )

# Global deps
# https://raw.github.com/stevedekorte/io/master/modules/FindGLIB2.cmake
set( GLIB2_FIND_REQUIRED ON )
find_package( GLIB2 )
if( NOT GLIB2_FOUND )
	message( FATAL_ERROR "Could not find GLIB2" )
endif()

# https://github.com/Pronghorn/pronghorn/blob/master/FindPROTOBUF-C.cmake
set( PROTOBUF_FIND_REQUIRED ON )
find_package( PROTOBUF-C )
if( NOT PROTOBUF_FOUND )
	message( FATAL_ERROR "Could not find protobuf-c" )
endif()

add_subdirectory( library/src )

if( BUILD_TESTS )
	#add_subdirectory( library/test )

	# enable testing for whole project
	enable_testing()
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
