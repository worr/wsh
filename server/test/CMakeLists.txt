set( TEST_EXECUTABLES test_run_cmd )

include_directories( ${CMAKE_SOURCE_DIR}/server/src ${WSH_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} )
foreach( TEST_EXECUTABLE ${TEST_EXECUTABLES} )
	add_executable( ${TEST_EXECUTABLE} ${TEST_EXECUTABLE}.c ${CMAKE_SOURCE_DIR}/server/src/cmd.c )
	add_test( ${TEST_EXECUTABLE} ${TEST_EXECUTABLE} )
	target_link_libraries( ${TEST_EXECUTABLE} ${WSH_LIBRARIES} ${GLIB2_LIBRARIES} )
endforeach( TEST_EXECUTABLE )
