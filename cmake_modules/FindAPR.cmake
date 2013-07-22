if( LIBAPR_INCLUDE_DIR AND LIBAPR_LIBRARY )
	set( LIBAPR_FIND_QUIETLY true )
endif( LIBAPR_INCLUDE_DIR AND LIBAPR_LIBRARY )

find_path( LIBAPR_INCLUDE_DIR
	NAMES apr_pools.h
	PATHS /usr/include /usr/include/apr /usr/include/apr-1 )

find_library( LIBAPR_LIBRARY NAMES apr-1 )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LibAPR DEFAULT_MSG LIBAPR_INCLUDE_DIR LIBAPR_LIBRARY )

mark_as_advanced( LIBAPR_INCLUDE_DIR LIBAPR_LIBRARY )
