add_executable( wshc main.c )

include_directories(
	${WSH_INCLUDE_DIRS}
	${GLIB2_INCLUDE_DIRS}
	${LIBSSH2_INCLUDE_DIR}
	${PROTOBUF_INCLUDE_DIR}
)

target_link_libraries( wshc
	${WSH_LIBRARIES}
	${GLIB2_LIBRARIES}
	${PROTOBUF_LIBRARIES}
	${LIBSSH2_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)
