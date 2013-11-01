option( COVERAGE "Build with gprov information" OFF )

if( COVERAGE )
	if( NOT CLANG AND NOT GCC )
		if( NOT LCOV )
			message( FATAL_ERROR "coverage not available outside of gcc and clang" )
		endif( NOT LCOV )
	endif( NOT CLANG AND NOT GCC )


	find_program( GCOV
		gcov
		/usr/bin
		/usr/local/bin
		/usr/pkg/bin
		/usr/sfw/bin
		/usr/xpg6/bin
		/usr/xpg4/bin
		/opt/csw/bin
	)

	find_program( LCOV
		lcov
		/usr/bin
		/usr/local/bin
		/usr/pkg/bin
		/usr/sfw/bin
		/usr/xpg6/bin
		/usr/xpg4/bin
		/opt/csw/bin
	)

	find_program( GENHTML
		genhtml
		/usr/bin
		/usr/local/bin
		/usr/pkg/bin
		/usr/sfw/bin
		/usr/xpg6/bin
		/usr/xpg4/bin
		/opt/csw/bin
	)

	find_program( FIND
		find
		/bin
		/usr/bin
		/usr/local/bin
		/usr/pkg/bin
		/usr/sfw/bin
		/usr/xpg6/bin
		/usr/xpg4/bin
		/opt/csw/bin
	)

	if( NOT LCOV )
		message( FATAL_ERROR "Could not find lcov" )
	endif( NOT LCOV )

	if( NOT GENHTML )
		message( FATAL_ERROR "Could not find genhtml" )
	endif( NOT GENHTML )

	if( NOT GCOV )
		message( FATAL_ERROR "Could not find gcov" )
	endif( NOT GCOV )

	if( NOT FIND )
		message( FATAL_ERROR "Could not find find" )
	endif( NOT FIND )

	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -Wno-error -fprofile-arcs -ftest-coverage" )
	set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage" )

	add_custom_target( coverage
		COMMAND lcov --zerocounters --directory .
		COMMAND lcov --capture --initial --directory . --output-file wsh
		COMMAND ${CMAKE_MAKE_PROGRAM} test
		COMMAND lcov --no-checksum --directory . --capture --output-file wsh.info
		COMMAND genhtml -o .. wsh.info
	)

	file( GLOB_RECURSE lcov_files ${CMAKE_CURRENT_SOURCE_DIR}/*.html ${CMAKE_CURRENT_SOURCE_DIR}/*.css ${CMAKE_CURRENT_SOURCE_DIR}/*.png )
	message( ${lcov_files} )
	add_custom_target( coverage_clean
		COMMAND rm ${lcov_files}
	)

endif( COVERAGE )
