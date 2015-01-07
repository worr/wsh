cmake_minimum_required( VERSION 2.6 )
project( wsh C )

set( APPLICATION_VERSION_MAJOR "1" )
set( APPLICATION_VERSION_MINOR "0" )
set( APPLICATION_VERSION_PATCH "0" )
set( APPLICATION_VERSION_EXTRA "rc5" )

set( APPLICATION_VERSION
	"${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}.${APPLICATION_VERSION_PATCH}${APPLICATION_VERSION_EXTRA}"
)

set( LIBRARY_VERSION
	"${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}.${APPLICATION_VERSION_PATCH}"
)

set( LIBRARY_SOVERSION "${APPLICATION_VERSION_MAJOR}" )

option( BUILD_SERVER "Build wsh server" ON )
option( BUILD_CLIENT "Build wsh client" ON )
option( BUILD_UTILS  "Build wsh utilities" ON )
option( BUILD_TESTS  "Build wsh unit tests" OFF )
option( WITH_RANGE   "Use libcrange as an host expansion lib" OFF )

set( LIBEXEC_PATH "${CMAKE_INSTALL_PREFIX}/libexec" CACHE STRING "Location for helper programs" )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules )

include( CheckCCompilerFlag )
include( CheckFunctionExists )
include( CheckIncludeFile )
include( CheckLibraryExists )
include( CheckSymbolExists )
include( Compiler )
include( Coverage )
include( CPack )
include( MacroEnsureOutOfSourceBuild )
include( OS )

if( LINKEDIN )
	include( LinkedIn )
endif( LINKEDIN )

set( GLIB2_MIN_VERSION "2.32.0" )

macro_ensure_out_of_source_build( "You must build outside of the source tree" )

set( CMAKE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib )

if( OPENBSD )
	set( CMAKE_INSTALL_FULL_MANDIR ${CMAKE_INSTALL_PREFIX} )
else( OPENBSD )
	set( CMAKE_INSTALL_FULL_MANDIR ${CMAKE_INSTALL_PREFIX}/share )
endif( OPENBSD )

set( WSH_INCLUDE_DIRS ${CMAKE_OUTPUT_DIRECTORY}/gen ${CMAKE_SOURCE_DIR}/library/src )
set( WSH_LIBRARIES wsh )

check_include_file( termios.h HAVE_TERMIOS_H )
check_include_file( unistd.h HAVE_UNISTD_H )

if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
	add_definitions( -DDEBUG )
endif( CMAKE_BUILD_TYPE STREQUAL "Debug" )

# Global deps
# https://raw.github.com/stevedekorte/io/master/modules/FindGLIB2.cmake
find_package( GLIB2 REQUIRED )

# https://github.com/Pronghorn/pronghorn/blob/master/FindPROTOBUF-C.cmake
find_package( PROTOBUF-C REQUIRED )

find_package( Threads REQUIRED )
find_package( SSH REQUIRED )

find_package( Curses )
check_include_file( term.h HAVE_TERM_H )

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
endif( HAVE_C11 )

set( CMAKE_REQUIRED_INCLUDES ${LIBSSH_INCLUDE_DIR} )
set( CMAKE_REQUIRED_LIBRARIES ${LIBSSH_LIBRARY} ${LIBSSH_THREADS_LIBRARY} )

configure_file( ${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_SOURCE_DIR}/config.h )

# We need to set these per target
set( ORIG_CFLAGS ${CMAKE_C_FLAGS} )
set( CMAKE_C_FLAGS "${ORIG_CFLAGS} ${WSH_LIB_COMPILER_FLAGS}" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${WSH_EXE_LINKER_FLAGS}" )
set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${WSH_LIB_LINKER_FLAGS}" )

add_subdirectory( library/src )

if( BUILD_TESTS )
	# enable testing for whole project
	enable_testing()

	add_subdirectory( library/test )
endif( BUILD_TESTS )

if( BUILD_UTILS )
	set( CMAKE_C_FLAGS "${ORIG_CFLAGS} ${WSH_EXE_COMPILER_FLAGS}" )
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
	set( CMAKE_C_FLAGS "${ORIG_CFLAGS} ${WSH_EXE_COMPILER_FLAGS}" )
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
	set( CMAKE_C_FLAGS "${ORIG_CFLAGS} ${WSH_EXE_COMPILER_FLAGS}" )
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

add_custom_target( indent
	COMMAND find ../client ../server ../library -name '*.[hc]' -print0 | xargs -0
	astyle --style=google --indent=tab -xC80 -xL --indent-cases
	--indent-switches
)
