
cmake_minimum_required (VERSION 3.8)

include(CheckCXXCompilerFlag) 
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

aux_source_directory(. SRC_LIST)
add_executable(rtsp2fmp4 ${SRC_LIST})
 
if (CMAKE_COMPILER_IS_GNUCXX)
	add_custom_target(COPY_CONFIG ALL) 
	add_custom_command (
	TARGET COPY_CONFIG
	COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/rtsp2fmp4/config.json ${CMAKE_CURRENT_BINARY_DIR}/config.json 
	POST_BUILD
	)
	add_definitions(-std=c++11)
	add_compile_options(-std=c++11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread" )
	set(WEBSOCKETPP_DIR  "/home/sam/websocketpp")
	set(BOOST_INCLUDE_DIR "/usr/local/boost/include")
	set(BOOST_LIBRARY_DIR "/usr/local/boost/lib")
	set(LIVE_LIBRARY_DIR "/usr/local/lib")
	include_directories( "/usr/include" "3rdparty" "3rdparty/live555/include" "${BOOST_INCLUDE_DIR}" "${WEBSOCKETPP_DIR}")
	# FIND_PACKAGE(Threads) 
	# LINK_DIRECTORIES("${LIVE_LIBRARY_DIR}" "${BOOST_LIBRARY_DIR}")
	target_link_libraries(rtsp2fmp4 
	${LIVE_LIBRARY_DIR}/libliveMedia.a
	${LIVE_LIBRARY_DIR}/libBasicUsageEnvironment.a
	${LIVE_LIBRARY_DIR}/libgroupsock.a
	${LIVE_LIBRARY_DIR}/libUsageEnvironment.a	
	${BOOST_LIBRARY_DIR}/libboost_date_time.a
	${BOOST_LIBRARY_DIR}/libboost_regex.a
	${BOOST_LIBRARY_DIR}/libboost_thread.a
	# ${CMAKE_THREAD_LIBS_INIT}
	)
elseif (MSVC)
	add_custom_target(COPY_CONFIG ALL) 
	add_custom_command (
	TARGET COPY_CONFIG
	COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/rtsp2fmp4/config.json ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/config.json 
	PRE_BUILD
	)
	set(WEBSOCKETPP_DIR  "D:/projects/websocketpp")
	set(BOOST_INCLUDE_DIR "D:/boost/boost_1_69_0")
	set(BOOST_LIBRARY_DIR "D:/boost/boost_1_69_0/lib64-msvc-14.1")
	set(LIVE_LIBRARY_DIR "${PROJECT_SOURCE_DIR}/rtsp2fmp4/3rdparty/live555/lib/msvc/${CMAKE_BUILD_TYPE}")
	include_directories("3rdparty/live555/include" "3rdparty" "${BOOST_INCLUDE_DIR}" "${WEBSOCKETPP_DIR}" )
	# link_directories("${LIVE_LIBRARY_DIR}" "${BOOST_LIBRARY_DIR}")
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_link_libraries(rtsp2fmp4  
		${LIVE_LIBRARY_DIR}/liveMedia.lib 
		${LIVE_LIBRARY_DIR}/BasicUsageEnvironment.lib 
		${LIVE_LIBRARY_DIR}/groupsock.lib 
		${LIVE_LIBRARY_DIR}/UsageEnvironment.lib 
		${BOOST_LIBRARY_DIR}/libboost_date_time-vc141-mt-gd-x64-1_69.lib 
		${BOOST_LIBRARY_DIR}/libboost_regex-vc141-mt-gd-x64-1_69.lib 
		)
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
		target_link_libraries(rtsp2fmp4 
		${LIVE_LIBRARY_DIR}/liveMedia.lib  
		${LIVE_LIBRARY_DIR}/BasicUsageEnvironment.lib 
		${LIVE_LIBRARY_DIR}/groupsock.lib 
		${LIVE_LIBRARY_DIR}/UsageEnvironment.lib 
		${BOOST_LIBRARY_DIR}/libboost_date_time-vc141-mt-x64-1_69.lib 
		${BOOST_LIBRARY_DIR}/libboost_regex-vc141-mt-x64-1_69.lib 
		)
	else()
		message("Build Type Not support:  ${CMAKE_BUILD_TYPE}")
	endif()
endif()



