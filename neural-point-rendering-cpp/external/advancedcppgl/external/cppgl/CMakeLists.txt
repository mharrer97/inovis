project(CppGL)
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

option(BUILD_CPPGL_EXAMPLES OFF)

# ---------------------------------------------------------------------
# compiler options

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

# setup for included dependencies
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_LINK_DIRECTORIES_BEFORE ON)

if(UNIX)
    set(LIN_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/thirdparty/lin/lib")
    execute_process(COMMAND "ln" "-s" "${LIN_LIB_PATH}/libglfw.so.3.3"     "${LIN_LIB_PATH}/libglfw.so")
    execute_process(COMMAND "ln" "-s" "${LIN_LIB_PATH}/libGLEW.so.2.1.0"   "${LIN_LIB_PATH}/libGLEW.so")
    execute_process(COMMAND "ln" "-s" "${LIN_LIB_PATH}/libGLEW.so.2.1.0"   "${LIN_LIB_PATH}/libGLEW.so.2.1")
    execute_process(COMMAND "ln" "-s" "${LIN_LIB_PATH}/libassimp.so.5.0.0" "${LIN_LIB_PATH}/libassimp.so")
    execute_process(COMMAND "ln" "-s" "${LIN_LIB_PATH}/libassimp.so.5.0.0" "${LIN_LIB_PATH}/libassimp.so.5")

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

include_directories(src)
add_subdirectory(src)

if (BUILD_CPPGL_EXAMPLES)
    add_subdirectory(examples)
endif()
