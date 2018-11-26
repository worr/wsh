if( LIBSSH_INCLUDE_DIR AND LIBSSH_LIBRARY )
	set( LIBSSH_FIND_QUIETLY true )
endif( LIBSSH_INCLUDE_DIR AND LIBSSH_LIBRARY )

find_path( LIBSSH_INCLUDE_DIR libssh/libssh.h )

find_library( LIBSSH_LIBRARY
		NAMES ssh
		PATHS /usr/local/lib /usr/local/lib64 /usr/pkg/lib /opt/lib /usr/lib64 /usr/lib /lib /lib64 /opt/pkg/lib
)

find_path( LIBSSH_INCLUDE_DIR libssh/libssh.h )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LibSSH DEFAULT_MSG LIBSSH_INCLUDE_DIR LIBSSH_LIBRARY )

mark_as_advanced( LIBSSH_INCLUDE_DIR LIBSSH_LIBRARY )
