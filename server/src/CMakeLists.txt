add_executable( wshd main.c parse.c output.c )
install( TARGETS wshd RUNTIME DESTINATION bin )

include_directories( ${WSH_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIR} ${CMAKE_SOURCE_DIR} )
target_link_libraries( wshd ${WSH_LIBRARIES} ${GLIB2_LIBRARIES} ${PROTOBUF_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )
