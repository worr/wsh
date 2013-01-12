add_executable( wshd cmd.c main.c )

if( BUILD_TESTS )
	set_target_properties( wshd PROPERTIES ENABLE_EXPORTS on )
endif( BUILD_TESTS )

include_directories( ${WSH_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIR} )
target_link_libraries( wshd ${WSH_LIBRARIES} ${GLIB2_LIBRARIES} ${PROTOBUF_LIBRARIES} )
