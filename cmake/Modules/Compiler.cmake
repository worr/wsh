include( CheckC11 )
include( CheckCCompilerFlag )

if( CMAKE_COMPILER_IS_GNUCC )
	set( GCC 1 )
endif (CMAKE_COMPILER_IS_GNUCC )

if( __COMPILER_CLANG )
	set( CLANG 1 )
endif( __COMPILER_CLANG )

if( CMAKE_C_COMPILER_ID EQUAL SunPro )
	set( SUNSTUDIO 1 )
endif( CMAKE_C_COMPILER_ID EQUAL SunPro )

if( CLANG OR GCC )
	check_c11( )
	if( HAVE_C11 )
		add_definitions( -std=c11 )
	else( HAVE_C11 )
		add_definitions( -std=c99 )
	endif( HAVE_C11 )

	add_definitions( -Wall -pedantic )

	check_c_compiler_flag( -Wno-pointer-sign HAVE_WNO_POINTER_SIGN )
	if( HAVE_WNO_POINTER_SIGN )
		add_definitions( -Wno-pointer-sign )
		add_definitions( -Werror )
	endif( HAVE_WNO_POINTER_SIGN )
endif( CLANG OR GCC )
