add_executable( wsh-add-hostkeys wsh_add_hostkeys.c )
install( TARGETS wsh-add-hostkeys RUNTIME DESTINATION bin )

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
