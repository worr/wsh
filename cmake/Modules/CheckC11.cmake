include( CheckCSourceCompiles )

macro( check_c11 )
	set( CMAKE_REQUIRED_FLAGS "-std=c11" )
	check_c_source_compiles( "int main(void) { return 0; }" HAVE_C11 )
	set( CMAKE_REQUIRED_FLAGS "" )

	if( HAVE_C11 )
		list(APPEND CMAKE_REQUIRED_DEFINITIONS -D__STDC_WANT_LIB_EXT1__ )
	endif( HAVE_C11 )
endmacro( check_c11 )
