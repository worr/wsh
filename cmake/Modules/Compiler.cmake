include( CheckC11 )
include( CheckCCompilerFlag )

set( WSH_EXE_COMPILER_FLAGS "" )
set( WSH_LIB_COMPILER_FLAGS "" )
set( WSH_EXE_LINKER_FLAGS "" )
set( WSH_LIB_LINKER_FLAGS "" )

if( CMAKE_COMPILER_IS_GNUCC )
	set( GCC 1 )
endif (CMAKE_COMPILER_IS_GNUCC )

if( __COMPILER_CLANG )
	set( CLANG 1 )
endif( __COMPILER_CLANG )

if( CMAKE_C_COMPILER_ID EQUAL SunPro )
	set( SUNSTUDIO 1 )
endif( CMAKE_C_COMPILER_ID EQUAL SunPro )

check_c11( )
if( HAVE_C11 )
	list( APPEND CMAKE_REQUIRED_FLAGS -std=c11 )
else( HAVE_C11 )
	list( APPEND CMAKE_REQUIRED_FLAGS -std=c99 )
endif( HAVE_C11 )

list( APPEND CMAKE_REQUIRED_DEFINITIONS -Wall )

if( CLANG OR GCC )
	list( APPEND CMAKE_REQUIRED_FLAGS -pedantic )

	check_c_compiler_flag( -fPIC HAVE_FPIC )
	if( HAVE_FPIC )
		set( WSH_LIB_COMPILER_FLAGS "${WSH_LIB_COMPILER_FLAGS} -fPIC" )
	endif( HAVE_FPIC )

	check_c_compiler_flag( -pic HAVE_PIC )
	if( HAVE_PIC )
		set( WSH_LIB_LINKER_FLAGS "${WSH_LIB_LINKER_FLAGS} -pic" )
	endif( HAVE_PIC )

	check_c_compiler_flag( -fPIE HAVE_FPIE )
	if( HAVE_FPIE )
		set( WSH_EXE_COMPILER_FLAGS "${WSH_EXE_COMPILER_FLAGS} -fPIE" )
	endif( HAVE_FPIE )

	check_c_compiler_flag( -pie HAVE_PIE )
	if( HAVE_PIE )
		set( WSH_EXE_LINKER_FLAGS "${WSH_EXE_LINKER_FLAGS} -pie" )
	endif( HAVE_PIE )

	check_c_compiler_flag( -fstack-protector-all HAVE_STACK_PROTECTOR_ALL )
	if ( HAVE_STACK_PROTECTOR_ALL )
		list( APPEND CMAKE_REQUIRED_DEFINITIONS -fstack-protector-all )
	endif( HAVE_STACK_PROTECTOR_ALL )
endif( CLANG OR GCC )
