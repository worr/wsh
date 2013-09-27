cmake_minimum_required( VERSION 2.6 )
project( wsh C )

option( BUILD_SERVER "Build wsh server" ON )
option( BUILD_CLIENT "Build wsh client" ON )
option( BUILD_UTILS  "Build wsh utilities" ON )
option( BUILD_TESTS  "Build wsh unit tests" OFF )
option( WITH_RANGE   "Use libcrange as an host expansion lib" OFF )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules )

include( CheckC11 )
include( CheckCCompilerFlag )
include( CheckFunctionExists )
include( CPack )
include( MacroEnsureOutOfSourceBuild )

macro_ensure_out_of_source_build( "You must build outside of the source tree" )

set( CMAKE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )

set( CMAKE_INSTALL_FULL_MANDIR ${CMAKE_INSTALL_PREFIX}/share )

set( WSH_INCLUDE_DIRS ${CMAKE_OUTPUT_DIRECTORY}/gen ${CMAKE_SOURCE_DIR}/library/src )
set( WSH_LIBRARIES wsh )

if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
	add_definitions( -DDEBUG )
endif( CMAKE_BUILD_TYPE STREQUAL "Debug" )

check_c_compiler_flag( -std=c99 HAVE_C_STD_FLAG )
if( HAVE_C_STD_FLAG )
	check_c11( )
	if( HAVE_C11 )
		add_definitions( -std=c11 )
	else( HAVE_C11 )
		add_definitions( -std=c99 )
	endif( HAVE_C11 )
endif( HAVE_C_STD_FLAG )

check_c_compiler_flag( -Wall HAVE_WALL )
if( HAVE_WALL )
	add_definitions( -Wall -pedantic )
endif( HAVE_WALL )

check_c_compiler_flag( -Wno-pointer-sign HAVE_WNO_POINTER_SIGN )
if( HAVE_WNO_POINTER_SIGN )
	add_definitions( -Wno-pointer-sign )
endif( HAVE_WNO_POINTER_SIGN )

add_definitions( -D_POSIX_C_SOURCE=200809L -D_SVID_SOURCE )

if( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )
	add_definitions( -D__EXTENSIONS__ )
endif( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )


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
if( NOT LIB_INSTALL_DIR )
	set( libdir "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" )
else( NOT LIB_INSTALL_DIR )
	set( libdir "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}" )
endif( NOT LIB_INSTALL_DIR )

set( CMAKE_INSTALL_RPATH "${libdir}" )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )
list( FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
	"${libdir}" isSystemDir )
if( "${isSystemDir}" STREQUAL "-1" )
	set( CMAKE_INSTALL_RPATH "${libdir}" )
endif( "${isSystemDir}" STREQUAL "-1" )

# This is a hack to get around problems with the custom cmake libs I found
if( IS_DIRECTORY /usr/local/lib )
	link_directories( /usr/local/lib )
endif( IS_DIRECTORY /usr/local/lib )

if( IS_DIRECTORY /usr/pkg/lib )
	link_directories( /usr/pkg/lib )
endif( IS_DIRECTORY /usr/pkg/lib )

# Function tests
if( HAVE_C11 )
	check_function_exists( memset_s HAVE_MEMSET_S )
	if( HAVE_MEMSET_S )
		add_definitions( -DHAVE_MEMSET_S )
	endif( HAVE_MEMSET_S )
endif( HAVE_C11 )

set( CMAKE_REQUIRED_INCLUDES ${LIBSSH_INCLUDE_DIR} )
set( CMAKE_REQUIRED_LIBRARIES ${LIBSSH_LIBRARY} )
check_function_exists( ssh_userauth_none HAVE_SSH_USERAUTH_NONE )
if( HAVE_SSH_USERAUTH_NONE )
	add_definitions( -DHAVE_SSH_USERAUTH_NONE )
endif( HAVE_SSH_USERAUTH_NONE )

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
	install(
		FILES man/wsh-add-hostkeys.1
		DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man/man1
		COMPONENT doc
	)
endif( BUILD_UTILS )

if( BUILD_SERVER )
	add_subdirectory( server/src )
	if( BUILD_TESTS )
		add_subdirectory( server/test )
	endif( BUILD_TESTS )
	install(
		FILES man/wshd.1
		DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man/man1
		COMPONENT doc
	)
endif( BUILD_SERVER )

if( BUILD_CLIENT )
	add_subdirectory( client/src )
	if( BUILD_TESTS )
		add_subdirectory( client/test )
	endif( BUILD_TESTS )

	install(
		FILES man/wshc.1 man/wscp.1
		DESTINATION ${CMAKE_INSTALL_FULL_MANDIR}/man/man1
		COMPONENT doc
	)
endif( BUILD_CLIENT )

## Build docs

add_custom_target( doc 
	COMMAND doxygen ../Doxyfile
)
