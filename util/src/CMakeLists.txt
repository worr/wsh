add_executable( wsh-add-hostkeys wsh_add_hostkeys.c )

include_directories( ${WSH_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIR} )
target_link_libraries( wsh-add-hostkeys ${WSH_LIBRARIES} ${GLIB2_LIBRARIES} ${PROTOBUF_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )
