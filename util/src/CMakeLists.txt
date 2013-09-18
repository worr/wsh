set( WSH_ASKPASS_SOURCES wsh_askpass.c )
if( NOT HAVE_MEMSET_S )
	set( WSH_ASKPASS_SOURCES ${WSH_ASKPASS_SOURCES} memset_s.c )
endif( NOT HAVE_MEMSET_S )

add_executable( wsh-add-hostkeys wsh_add_hostkeys.c )
add_executable( wsh-askpass memset_s.c wsh_askpass.c )
add_executable( wscp wscp.c )

install( TARGETS wsh-add-hostkeys RUNTIME DESTINATION bin )
install( TARGETS wsh-askpass RUNTIME DESTINATION libexec )
install( TARGETS wscp RUNTIME DESTINATION bin )

INCLUDE_Directories(
	${WSH_INCLUDE_DIRS}
	${GLIB2_INCLUDE_DIRS}
	${PROTOBUF_INCLUDE_DIR}
)

target_link_libraries( wsh-add-hostkeys
	${WSH_LIBRARIES}
	${GTHREAD2_LIBRARIES}
	${GLIB2_LIBRARIES}
	${PROTOBUF_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries( wscp
	${WSH_LIBRARIES}
	${GTHREAD2_LIBRARIES}
	${GLIB2_LIBRARIES}
	${PROTOBUF_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)
