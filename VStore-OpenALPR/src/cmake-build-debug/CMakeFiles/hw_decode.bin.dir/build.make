# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.9

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/teddyxu/Documents/clion-2017.3/bin/cmake/bin/cmake

# The command to remove a file.
RM = /home/teddyxu/Documents/clion-2017.3/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/hw_decode.bin.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/hw_decode.bin.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/hw_decode.bin.dir/flags.make

CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o: CMakeFiles/hw_decode.bin.dir/flags.make
CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o: ../examples/hw_decode.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o   -c /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/examples/hw_decode.c

CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.i"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/examples/hw_decode.c > CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.i

CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.s"
	gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/examples/hw_decode.c -o CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.s

CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o.requires:

.PHONY : CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o.requires

CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o.provides: CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o.requires
	$(MAKE) -f CMakeFiles/hw_decode.bin.dir/build.make CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o.provides.build
.PHONY : CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o.provides

CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o.provides.build: CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o


# Object files for target hw_decode.bin
hw_decode_bin_OBJECTS = \
"CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o"

# External object files for target hw_decode.bin
hw_decode_bin_EXTERNAL_OBJECTS =

hw_decode.bin: CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o
hw_decode.bin: CMakeFiles/hw_decode.bin.dir/build.make
hw_decode.bin: CMakeFiles/hw_decode.bin.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable hw_decode.bin"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/hw_decode.bin.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/hw_decode.bin.dir/build: hw_decode.bin

.PHONY : CMakeFiles/hw_decode.bin.dir/build

CMakeFiles/hw_decode.bin.dir/requires: CMakeFiles/hw_decode.bin.dir/examples/hw_decode.c.o.requires

.PHONY : CMakeFiles/hw_decode.bin.dir/requires

CMakeFiles/hw_decode.bin.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/hw_decode.bin.dir/cmake_clean.cmake
.PHONY : CMakeFiles/hw_decode.bin.dir/clean

CMakeFiles/hw_decode.bin.dir/depend:
	cd /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/cmake-build-debug /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/cmake-build-debug /home/teddyxu/Desktop/end2end-perf-test/e2ealpr/video-streamer/src/cmake-build-debug/CMakeFiles/hw_decode.bin.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/hw_decode.bin.dir/depend

