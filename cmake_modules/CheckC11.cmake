include( CheckCSourceCompiles )

macro( check_c11 )
	set( CMAKE_REQUIRED_FLAGS "-std=c11" )
	check_c_source_compiles( "int main(void) { return 0; }" HAVE_C11 )
	set( CMAKE_REQUIRED_FLAGS "" )
endmacro( check_c11 )
