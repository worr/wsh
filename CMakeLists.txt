cmake_minimum_required( VERSION 2.6 )
project( wsh C )

option( BUILD_SERVER "Build wsh server" ON )
option( BUILD_CLIENT "Build wsh client" ON )
option( BUILD_UTILS  "Build wsh utilities" ON )
option( BUILD_TESTS  "Build wsh unit tests" OFF )
option( WITH_RANGE   "Use libcrange as an host expansion lib" OFF )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules )

include( CheckFunctionExists )
include( CheckC11 )

set( CMAKE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )

set( WSH_INCLUDE_DIRS ${CMAKE_OUTPUT_DIRECTORY}/gen ${CMAKE_SOURCE_DIR}/library/src )
set( WSH_LIBRARIES wsh )

check_c11( )

if( HAVE_C11 )
	add_definitions( -std=c11 )
else( HAVE_C11 )
	add_definitions( -std=c99 )
endif( HAVE_C11 )

add_definitions( -Wall -Werror -pedantic )
add_definitions( -D_GNU_SOURCE )

if( WITH_RANGE )
	add_definitions( -DRANGE )
endif( WITH_RANGE )

# Global deps
# https://raw.github.com/stevedekorte/io/master/modules/FindGLIB2.cmake
find_package( GLIB2 REQUIRED )

# https://github.com/Pronghorn/pronghorn/blob/master/FindPROTOBUF-C.cmake
find_package( PROTOBUF-C REQUIRED )

find_package( Threads REQUIRED )
find_package( SSH REQUIRED )

# Optional dep
if( WITH_RANGE )
	find_package( CRANGE )
	if( LIBCRANGE_LIBRARY )
		find_package( APR REQUIRED )
	endif( LIBCRANGE_LIBRARY )
endif( WITH_RANGE )

# Deal with linker crap
set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )
list( FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
	"${CMAKE_INSTALL_PREFIX}/lib" isSystemDir )
if( "${isSystemDir}" STREQUAL "-1" )
	set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )
endif( "${isSystemDir}" STREQUAL "-1" )

# This is a hack to get around problems with the custom cmake libs I found
if( IS_DIRECTORY /usr/local/lib )
	link_directories( /usr/local/lib )
endif( IS_DIRECTORY /usr/local/lib )

if( IS_DIRECTORY /usr/pkg/lib )
	link_directories( /usr/pkg/lib )
endif( IS_DIRECTORY /usr/pkg/lib )

# Function tests
check_function_exists( mlock HAVE_MLOCK )
check_function_exists( memset_s HAVE_MEMSET_S )

add_subdirectory( library/src )

if( BUILD_TESTS )
	# enable testing for whole project
	enable_testing()

	add_definitions( -DBUILD_TESTS )
	add_subdirectory( library/test )
endif( BUILD_TESTS )

if( BUILD_UTILS )
	add_subdirectory( util/src )
	if( BUILD_TESTS )
		#add_subdirectory( util/test )
	endif( BUILD_TESTS )
endif( BUILD_UTILS )

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
