set( TEST_EXECUTABLES test_log test_run_cmd test_auth_proto )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/tests )

include_directories( ${CMAKE_OUTPUT_DIRECTORY}/gen ${CMAKE_SOURCE_DIR}/library/src ${CMAKE_SOURCE_DIR}/library/test/mock ${GLIB2_INCLUDE_DIRS} )
set( SOURCES 
	${CMAKE_OUTPUT_DIRECTORY}/gen/auth.pb-c.c
	${CMAKE_OUTPUT_DIRECTORY}/gen/cmd-messages.pb-c.c
	${CMAKE_SOURCE_DIR}/library/src/log.c
	${CMAKE_SOURCE_DIR}/library/src/cmd.c
	${CMAKE_SOURCE_DIR}/library/test/mock/syslog.c
)

foreach( TEST_EXECUTABLE ${TEST_EXECUTABLES} )
	add_executable( ${TEST_EXECUTABLE} ${TEST_EXECUTABLE}.c ${SOURCES} )
	add_test( ${TEST_EXECUTABLE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TEST_EXECUTABLE} )
	target_link_libraries( ${TEST_EXECUTABLE} ${GLIB2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${PROTOBUF_LIBRARIES} )
endforeach( TEST_EXECUTABLE )
