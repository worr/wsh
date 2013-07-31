set( PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/gen )
file( MAKE_DIRECTORY ${PROTO_GEN_DIR} )

set( TEST_EXECUTABLES test_log test_run_cmd test_auth_proto test_cmd_messages_proto test_pack test_ssh )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/tests )

include_directories( 
	${PROTO_GEN_DIR}
	${CMAKE_SOURCE_DIR}/library/src
	${CMAKE_SOURCE_DIR}/library/test/mock
	${GLIB2_INCLUDE_DIRS}
	${PROTOBUF_INCLUDE_DIR}
)

set( PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/gen )
set( WSH_PROTOC_SOURCES ${PROTO_GEN_DIR}/cmd-messages.pb-c.c ${PROTO_GEN_DIR}/auth.pb-c.c )
set_source_files_properties( ${WSH_PROTOC_SOURCES} PROPERTIES GENERATED TRUE )
set( SOURCES 
	${WSH_PROTOC_SOURCES}
	${CMAKE_SOURCE_DIR}/library/src/log.c
	${CMAKE_SOURCE_DIR}/library/src/cmd.c
	${CMAKE_SOURCE_DIR}/library/src/pack.c
	${CMAKE_SOURCE_DIR}/library/src/ssh.c
	${CMAKE_SOURCE_DIR}/library/test/mock/syslog.c
	${CMAKE_SOURCE_DIR}/library/test/mock/libssh/libssh.c
)

if( NOT HAVE_MEMSET_S )
	set( SOURCES ${CMAKE_SOURCE_DIR}/library/src/memset_s.c ${SOURCES} )
endif( NOT HAVE_MEMSET_S )

foreach( TEST_EXECUTABLE ${TEST_EXECUTABLES} )
	add_executable( ${TEST_EXECUTABLE} ${TEST_EXECUTABLE}.c ${SOURCES} )
	add_test( ${TEST_EXECUTABLE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TEST_EXECUTABLE} )
	target_link_libraries(
		${TEST_EXECUTABLE}
		${GLIB2_LIBRARIES}
		${CMAKE_THREAD_LIBS_INIT}
		${PROTOBUF_LIBRARIES}
	)
endforeach( TEST_EXECUTABLE )
