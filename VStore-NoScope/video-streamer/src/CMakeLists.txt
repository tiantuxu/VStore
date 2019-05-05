

cmake_minimum_required(VERSION 2.8.9)

project(video-streamer)

set(deploy_include_path "/shared/include")

################################
#  toolchain, dependency, etc.
################################

SET( CMAKE_CXX_COMPILER "/usr/bin/g++-5"  )
SET( CMAKE_C_COMPILER "gcc"  )
#SET( CMAKE_CXX_COMPILER "icc" )

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")

SET(CMAKE_CXX_FLAGS_DEBUG
		"${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -g -O0 -Wall -Wfatal-errors")
SET(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -g -Wall -O3 -DHAVE_NUMA")

SET(CMAKE_C_FLAGS_DEBUG
		"${CMAKE_C_FLAGS_DEBUG} -std=gnu11 -g -O0 -Wall -Wfatal-errors")
SET(CMAKE_C_FLAGS_RELEASE
		"${CMAKE_C_FLAGS_RELEASE} -std=gnu11 -g -O3 -Wall -Wfatal-errors")

set(deploy_target_path "localhost:/tmp/$ENV{USER}")

################################
#  headers, source
################################

# add required libraries
# all libraries are in /shared/hikey
include_directories(
        ./
        /shared/ffmpeg/include
        /shared/libzmq/include
        #/usr/local/include
		#/home/teddyxu/opencvlib/include
		/usr/include/opencv2
)

link_directories(
        /shared/ffmpeg/lib
        /shared/libzmq/lib
		#/usr/local/lib
		#/home/teddyxu/opencvlib/lib
		/usr/lib
)


file (GLOB HEADERS
        *.h
		include/*.h
		include/mm/*.h
		include/xplane/*.h
)

#SET(SOURCE
#    measure.c
#    mm.cpp
#    lmdb-if.cpp
#    ${HEADERS}
#    )

file (GLOB LIB_SOURCES
        ${HEADERS}
        measure.c
        mm.cpp
        mydecoder.cpp
        #        mydecoder_sw.cpp       # obsoleted
        rxtx.cpp
        lmdb-if.cpp
        stream-info.cpp
        )

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	#add_definitions(-D_GLIBCXX_DEBUG) # if the compliation unit use boost::, it's gonna break.
	add_definitions(-DCONFIG_KAGE_GLOBAL_DEBUG_LEVEL=30)
    add_definitions(-DDEBUG)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	add_definitions(-DCONFIG_KAGE_GLOBAL_DEBUG_LEVEL=50)
	add_definitions(-DNDEBUG)
endif ()

###########################
# libs
# singling these out as their compliation is slow.
###########################
add_library(vstreamer STATIC
    ${LIB_SOURCES})

SET (FFMPEG_LIBS
        avdevice
        avfilter
        avformat
        avcodec
        swresample
        swscale
        avutil
		postproc
		#X11 va-x11 va-drm  va # intel's video accel lib etc
		vdpau  # nvidia decoding & presentation
        )

SET (COMMON_LIBS
        m
        xcb
        xcb-shm
        xcb-shape
        dl
        bz2
        z
        pthread
        lzma
        boost_program_options
        boost_serialization
        X11
		opencv_core
		opencv_highgui
		opencv_imgproc
        )

find_package(OpenMP)
if (OPENMP_FOUND)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

#ADD_LIBRARY(gompdebug STATIC IMPORTED)
#SET_TARGET_PROPERTIES(gompdebug PROPERTIES IMPORTED_LOCATION /usr/lib/debug/usr/lib/x86_64-linux-gnu/libgomp.so.1.0.0)

#ADD_LIBRARY(myomp STATIC IMPORTED)
#SET_TARGET_PROPERTIES(myomp PROPERTIES IMPORTED_LOCATION /usr/lib/x86_64-linux-gnu/libgomp.so.1)
###########################

#SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/library")
#SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/library")
# for store static library to diff directory
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

#SET(CMAKE_C_FLAGS	"${CMAKE_C_FLAGS} -finline-functions")

#SET(CMAKE_EXE_LINKER_FLAGS	)

#FIND_LIBRARY(test NAMES libtest.a PATHS .)

#add_custom_command(TARGET xplane POST_BUILD
#		COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/lib/libxplane.a ${deploy_target_path}/lib/libxplane-${CMAKE_BUILD_TYPE}.a
#		COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/include/xplane_lib.h ${deploy_include_path}
#		COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/include/xplane/* ${deploy_include_path}/xplane/
#		)
#ADD_LIBRARY(opencv_core24 STATIC IMPORTED)
#SET_TARGET_PROPERTIES(opencv_core24 PROPERTIES IMPORTED_LOCATION /home/teddyxu/opencvlib/lib/libopencv_core.so)
#SET (COMMON_LIB ${COMMON_LIB} opencv_core)

#ADD_LIBRARY(opencv_highgui24 STATIC IMPORTED)
#SET_TARGET_PROPERTIES(opencv_highgui24 PROPERTIES IMPORTED_LOCATION /home/teddyxu/opencvlib/lib/libopencv_highgui.so)
#SET (COMMON_LIB ${COMMON_LIB} opencv_highgui)

#ADD_LIBRARY(opencv_imgproc24 STATIC IMPORTED)
#SET_TARGET_PROPERTIES(opencv_imgproc24 PROPERTIES IMPORTED_LOCATION /home/teddyxu/opencvlib/lib/libopencv_imgproc.so)
#SET (COMMON_LIB ${COMMON_LIB} opencv_imgproc)

#ADD_LIBRARY(myopenalpr STATIC IMPORTED)
#SET_TARGET_PROPERTIES(myopenalpr PROPERTIES IMPORTED_LOCATION /home/teddyxu/openalprlib/lib/libopenalpr.so)
#SET (COMMON_LIB ${COMMON_LIB} opencv_imgproc)

########################################
# Test
########################################

########################################
add_executable(hw_decode.bin
		examples/hw_decode.c
		)
TARGET_LINK_LIBRARIES(hw_decode.bin
        ${FFMPEG_LIBS}
        ${COMMON_LIBS}
		)
########################################
add_executable(demuxing_decoding.bin
        examples/demuxing_decoding.c
		)
TARGET_LINK_LIBRARIES(demuxing_decoding.bin
        ${FFMPEG_LIBS}
        ${COMMON_LIBS}
        )
########################################
add_executable(decode_video.bin
        examples/decode_video.c
        )
TARGET_LINK_LIBRARIES(decode_video.bin
        ${FFMPEG_LIBS}
        ${COMMON_LIBS}
        )
########################################
add_executable(taskvent.bin
        examples/taskvent.cpp
        )
TARGET_LINK_LIBRARIES(taskvent.bin
        zmq
        )
########################################
add_executable(taskwork2.bin
        examples/taskwork2.cpp
        )
TARGET_LINK_LIBRARIES(taskwork2.bin
        zmq
        )
########################################
#SET(PROGRAM test-sink)
#add_executable(${PROGRAM}.bin
#        test/${PROGRAM}.cpp
#        )
#TARGET_LINK_LIBRARIES(${PROGRAM}.bin
#        vstreamer
#        ${FFMPEG_LIBS}
#        ${COMMON_LIBS}
#        lmdb
#        zmq
#		openalpr
#		openalpr-static
#		opencv_core
#		opencv_highgui
#		opencv_imgproc
#        )
#add_custom_command(TARGET ${PROGRAM}.bin POST_BUILD
#        COMMAND scp ${PROGRAM}.bin ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
#        )
########################################
SET(BINARY test-source.bin)
add_executable(${BINARY}
        test/test-source.cpp
        )
TARGET_LINK_LIBRARIES(${BINARY}
        vstreamer
        ${FFMPEG_LIBS}
        ${COMMON_LIBS}
        lmdb
        zmq
		opencv_core
		opencv_highgui
		opencv_imgproc
        )
add_custom_command(TARGET ${BINARY} POST_BUILD
        COMMAND scp ${BINARY} ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
        )
########################################
SET(BINARY test-chunk-puller.bin)
add_executable(${BINARY}
        test/test-chunk-puller.cpp)
TARGET_LINK_LIBRARIES(${BINARY}
        vstreamer
        ${FFMPEG_LIBS}
        ${COMMON_LIBS}
        lmdb
        zmq
        )
add_custom_command(TARGET ${BINARY} POST_BUILD
        COMMAND scp ${BINARY} ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
        )
########################################
SET(PROGRAM decode-serv)
add_executable(${PROGRAM}.bin
        decode-serv.cpp
        )
TARGET_LINK_LIBRARIES(${PROGRAM}.bin
        vstreamer
        ${FFMPEG_LIBS}
        ${COMMON_LIBS}
        lmdb
        zmq
        )
add_custom_command(TARGET ${PROGRAM}.bin POST_BUILD
        COMMAND scp ${PROGRAM}.bin ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
        )
########################################
SET(PROGRAM test-fb-sender)
add_executable(${PROGRAM}.bin
        test/${PROGRAM}.cpp
        )
TARGET_LINK_LIBRARIES(${PROGRAM}.bin
        vstreamer
        ${FFMPEG_LIBS}
        ${COMMON_LIBS}
        lmdb
        zmq
        )
add_custom_command(TARGET ${PROGRAM}.bin POST_BUILD
        COMMAND scp ${PROGRAM}.bin ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
        )
########################################
SET(PROGRAM test-fb-recver)
add_executable(${PROGRAM}.bin
        test/${PROGRAM}.cpp
        )
TARGET_LINK_LIBRARIES(${PROGRAM}.bin
        vstreamer
        ${FFMPEG_LIBS}
        ${COMMON_LIBS}
        lmdb
        zmq
        )
add_custom_command(TARGET ${PROGRAM}.bin POST_BUILD
        COMMAND scp ${PROGRAM}.bin ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
        )
########################################
SET(PROGRAM test-decoder)
add_executable(${PROGRAM}.bin
        test/${PROGRAM}.cpp
        )
TARGET_LINK_LIBRARIES(${PROGRAM}.bin
        vstreamer
        ${FFMPEG_LIBS}
        ${COMMON_LIBS}
        lmdb
        zmq
        )
add_custom_command(TARGET ${PROGRAM}.bin POST_BUILD
        COMMAND scp ${PROGRAM}.bin ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
        )
########################################
SET(PROGRAM test-db-build)
add_executable(${PROGRAM}.bin
		test/${PROGRAM}.cpp
		)
TARGET_LINK_LIBRARIES(${PROGRAM}.bin
		vstreamer
		${FFMPEG_LIBS}
		${COMMON_LIBS}
		lmdb
		zmq
		)
add_custom_command(TARGET ${PROGRAM}.bin POST_BUILD
		COMMAND scp ${PROGRAM}.bin ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
		)
########################################
SET(PROGRAM test-rt-scheduler)
add_executable(${PROGRAM}.bin
		test/${PROGRAM}.cpp
		)
TARGET_LINK_LIBRARIES(${PROGRAM}.bin
		vstreamer
		${FFMPEG_LIBS}
		${COMMON_LIBS}
		lmdb
		zmq
		)
add_custom_command(TARGET ${PROGRAM}.bin POST_BUILD
		COMMAND scp ${PROGRAM}.bin ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
		)
########################################
#SET(PROGRAM test-migrate)
#add_executable(${PROGRAM}.bin
#		test/${PROGRAM}.cpp
#		)
#TARGET_LINK_LIBRARIES(${PROGRAM}.bin
#		vstreamer
#		${FFMPEG_LIBS}
#		${COMMON_LIBS}
#		lmdb
#		zmq
#		)
#add_custom_command(TARGET ${PROGRAM}.bin POST_BUILD
#		COMMAND scp ${PROGRAM}.bin ${deploy_target_path}/${CMAKE_BUILD_TYPE}/
#		)
########################################
