set( TEST_EXECUTABLES client_test_output )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/tests )

include_directories(
	${CMAKE_OUTPUT_DIRECTORY}/gen
	${CMAKE_SOURCE_DIR}/client/src
	${CMAKE_SOURCE_DIR}/library/src
	${CMAKE_SOURCE_DIR}/library/test/mock
	${CMAKE_SOURCE_DIR}/client/test/mock
	${GLIB2_INCLUDE_DIRS}
	${PROTOBUF_INCLUDE_DIR}
)

set( PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/gen )
set( WSH_PROTOC_SOURCES ${PROTO_GEN_DIR}/cmd-messages.pb-c.c ${PROTO_GEN_DIR}/auth.pb-c.c )
set_source_files_properties( ${WSH_PROTOC_SOURCES} PROPERTIES GENERATED TRUE )
set( SOURCES 
	${WSH_PROTOC_SOURCES}
	${CMAKE_SOURCE_DIR}/client/src/output.c
	${CMAKE_SOURCE_DIR}/client/test/mock/isatty.c
)

foreach( TEST_EXECUTABLE ${TEST_EXECUTABLES} )
	add_executable( ${TEST_EXECUTABLE} ${TEST_EXECUTABLE}.c ${SOURCES} )
	add_test( ${TEST_EXECUTABLE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TEST_EXECUTABLE} )
	add_definitions( -DUNIT_TESTING )
	target_link_libraries( ${TEST_EXECUTABLE}
		${WSH_LIBRARIES}
		${GLIB2_LIBRARIES}
		${GTHREAD2_LIBRARIES}
		${CMAKE_THREAD_LIBS_INIT}
		${PROTOBUF_LIBRARIES}
	)
endforeach( TEST_EXECUTABLE )
