# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.0

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/clayton/Desktop/cutecom-0.22.0

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/clayton/Desktop/cutecom-0.22.0

# Include any dependencies generated for this target.
include CMakeFiles/cutecom.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/cutecom.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/cutecom.dir/flags.make

moc_qcppdialogimpl.cxx: qcppdialogimpl.h
	$(CMAKE_COMMAND) -E cmake_progress_report /home/clayton/Desktop/cutecom-0.22.0/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold "Generating moc_qcppdialogimpl.cxx"
	/usr/bin/moc @/home/clayton/Desktop/cutecom-0.22.0/moc_qcppdialogimpl.cxx_parameters

ui_cutecommdlg.h: cutecommdlg.ui
	$(CMAKE_COMMAND) -E cmake_progress_report /home/clayton/Desktop/cutecom-0.22.0/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold "Generating ui_cutecommdlg.h"
	/usr/bin/uic -o /home/clayton/Desktop/cutecom-0.22.0/ui_cutecommdlg.h /home/clayton/Desktop/cutecom-0.22.0/cutecommdlg.ui

CMakeFiles/cutecom.dir/main.o: CMakeFiles/cutecom.dir/flags.make
CMakeFiles/cutecom.dir/main.o: main.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/clayton/Desktop/cutecom-0.22.0/CMakeFiles $(CMAKE_PROGRESS_3)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/cutecom.dir/main.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/cutecom.dir/main.o -c /home/clayton/Desktop/cutecom-0.22.0/main.cpp

CMakeFiles/cutecom.dir/main.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cutecom.dir/main.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/clayton/Desktop/cutecom-0.22.0/main.cpp > CMakeFiles/cutecom.dir/main.i

CMakeFiles/cutecom.dir/main.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cutecom.dir/main.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/clayton/Desktop/cutecom-0.22.0/main.cpp -o CMakeFiles/cutecom.dir/main.s

CMakeFiles/cutecom.dir/main.o.requires:
.PHONY : CMakeFiles/cutecom.dir/main.o.requires

CMakeFiles/cutecom.dir/main.o.provides: CMakeFiles/cutecom.dir/main.o.requires
	$(MAKE) -f CMakeFiles/cutecom.dir/build.make CMakeFiles/cutecom.dir/main.o.provides.build
.PHONY : CMakeFiles/cutecom.dir/main.o.provides

CMakeFiles/cutecom.dir/main.o.provides.build: CMakeFiles/cutecom.dir/main.o

CMakeFiles/cutecom.dir/qcppdialogimpl.o: CMakeFiles/cutecom.dir/flags.make
CMakeFiles/cutecom.dir/qcppdialogimpl.o: qcppdialogimpl.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/clayton/Desktop/cutecom-0.22.0/CMakeFiles $(CMAKE_PROGRESS_4)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/cutecom.dir/qcppdialogimpl.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/cutecom.dir/qcppdialogimpl.o -c /home/clayton/Desktop/cutecom-0.22.0/qcppdialogimpl.cpp

CMakeFiles/cutecom.dir/qcppdialogimpl.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cutecom.dir/qcppdialogimpl.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/clayton/Desktop/cutecom-0.22.0/qcppdialogimpl.cpp > CMakeFiles/cutecom.dir/qcppdialogimpl.i

CMakeFiles/cutecom.dir/qcppdialogimpl.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cutecom.dir/qcppdialogimpl.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/clayton/Desktop/cutecom-0.22.0/qcppdialogimpl.cpp -o CMakeFiles/cutecom.dir/qcppdialogimpl.s

CMakeFiles/cutecom.dir/qcppdialogimpl.o.requires:
.PHONY : CMakeFiles/cutecom.dir/qcppdialogimpl.o.requires

CMakeFiles/cutecom.dir/qcppdialogimpl.o.provides: CMakeFiles/cutecom.dir/qcppdialogimpl.o.requires
	$(MAKE) -f CMakeFiles/cutecom.dir/build.make CMakeFiles/cutecom.dir/qcppdialogimpl.o.provides.build
.PHONY : CMakeFiles/cutecom.dir/qcppdialogimpl.o.provides

CMakeFiles/cutecom.dir/qcppdialogimpl.o.provides.build: CMakeFiles/cutecom.dir/qcppdialogimpl.o

CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o: CMakeFiles/cutecom.dir/flags.make
CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o: moc_qcppdialogimpl.cxx
	$(CMAKE_COMMAND) -E cmake_progress_report /home/clayton/Desktop/cutecom-0.22.0/CMakeFiles $(CMAKE_PROGRESS_5)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o -c /home/clayton/Desktop/cutecom-0.22.0/moc_qcppdialogimpl.cxx

CMakeFiles/cutecom.dir/moc_qcppdialogimpl.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/cutecom.dir/moc_qcppdialogimpl.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/clayton/Desktop/cutecom-0.22.0/moc_qcppdialogimpl.cxx > CMakeFiles/cutecom.dir/moc_qcppdialogimpl.i

CMakeFiles/cutecom.dir/moc_qcppdialogimpl.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/cutecom.dir/moc_qcppdialogimpl.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/clayton/Desktop/cutecom-0.22.0/moc_qcppdialogimpl.cxx -o CMakeFiles/cutecom.dir/moc_qcppdialogimpl.s

CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o.requires:
.PHONY : CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o.requires

CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o.provides: CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o.requires
	$(MAKE) -f CMakeFiles/cutecom.dir/build.make CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o.provides.build
.PHONY : CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o.provides

CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o.provides.build: CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o

# Object files for target cutecom
cutecom_OBJECTS = \
"CMakeFiles/cutecom.dir/main.o" \
"CMakeFiles/cutecom.dir/qcppdialogimpl.o" \
"CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o"

# External object files for target cutecom
cutecom_EXTERNAL_OBJECTS =

cutecom: CMakeFiles/cutecom.dir/main.o
cutecom: CMakeFiles/cutecom.dir/qcppdialogimpl.o
cutecom: CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o
cutecom: CMakeFiles/cutecom.dir/build.make
cutecom: /usr/lib64/libQt3Support.so
cutecom: /usr/lib64/libQtGui.so
cutecom: /usr/lib64/libQtCore.so
cutecom: CMakeFiles/cutecom.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable cutecom"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/cutecom.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/cutecom.dir/build: cutecom
.PHONY : CMakeFiles/cutecom.dir/build

CMakeFiles/cutecom.dir/requires: CMakeFiles/cutecom.dir/main.o.requires
CMakeFiles/cutecom.dir/requires: CMakeFiles/cutecom.dir/qcppdialogimpl.o.requires
CMakeFiles/cutecom.dir/requires: CMakeFiles/cutecom.dir/moc_qcppdialogimpl.o.requires
.PHONY : CMakeFiles/cutecom.dir/requires

CMakeFiles/cutecom.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/cutecom.dir/cmake_clean.cmake
.PHONY : CMakeFiles/cutecom.dir/clean

CMakeFiles/cutecom.dir/depend: moc_qcppdialogimpl.cxx
CMakeFiles/cutecom.dir/depend: ui_cutecommdlg.h
	cd /home/clayton/Desktop/cutecom-0.22.0 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/clayton/Desktop/cutecom-0.22.0 /home/clayton/Desktop/cutecom-0.22.0 /home/clayton/Desktop/cutecom-0.22.0 /home/clayton/Desktop/cutecom-0.22.0 /home/clayton/Desktop/cutecom-0.22.0/CMakeFiles/cutecom.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/cutecom.dir/depend

