add_executable( wshc main.c remote.c )

include_directories(
	${WSH_INCLUDE_DIRS}
	${GLIB2_INCLUDE_DIRS}
	${LIBSSH_INCLUDE_DIR}
	${PROTOBUF_INCLUDE_DIR}
)

target_link_libraries( wshc
	${WSH_LIBRARIES}
	${GLIB2_LIBRARIES}
	${GTHREAD2_LIBRARIES}
	${PROTOBUF_LIBRARIES}
	${LIBSSH_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)
