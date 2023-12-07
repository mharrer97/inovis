project(AdvancedCppGL)
cmake_minimum_required(VERSION 3.10)

if (NOT UNIX AND NOT WIN32)
	message(FATAL_ERROR "Unsupported platform!")
endif()

# ---------------------------------------------------------------------
# path management

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# ---------------------------------------------------------------------
# cmake options

option(BUILD_ADVCPPGL_EXAMPLES OFF)

# ---------------------------------------------------------------------
# compiler options

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# setup for included dependencies
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_LINK_DIRECTORIES_BEFORE ON)

if(UNIX)
    add_compile_options(-pthread -march=native)
    add_compile_options(-Wall -Wno-unused-parameter -Wno-unused-variable -Wno-int-to-pointer-cast)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    add_definitions(-DNOMINMAX)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING) # wtf
    #with cmake version >3.8, CMAKE_INTERPROCEDURAL_OPTIMIZATION ON breaks windows dll export
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()


include(GenerateExportHeader)

# ---------------------------------------------------------------------
# traverse source tree

#build cppgl
include_directories(external/cppgl/src)
add_subdirectory(external/cppgl)

include_directories(src)
add_subdirectory(src)

if (BUILD_ADVCPPGL_EXAMPLES)
    add_subdirectory(examples)
endif()
