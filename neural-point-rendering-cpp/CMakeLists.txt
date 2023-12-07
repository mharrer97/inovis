cmake_minimum_required(VERSION 3.10)
project(Inovis LANGUAGES CUDA CXX)


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

option(BUILD_ADVCPPGL_EXAMPLES ON)

# ---------------------------------------------------------------------
# compiler options

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# setup for included dependencies
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_LINK_DIRECTORIES_BEFORE ON)

if(UNIX)
    message("Set UNIX Compiler Flags")
    add_compile_options(-pthread -march=native)
    add_compile_options(-Wall -Wno-unused-parameter -Wno-unused-variable -Wno-int-to-pointer-cast)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    message("Set MVSC Compiler Flags")
    add_definitions(-DNOMINMAX)
    add_definitions(-D_USE_MATH_DEFINES) 
    add_definitions(-D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING) # wtf
    #with cmake version >3.8, CMAKE_INTERPROCEDURAL_OPTIMIZATION ON breaks windows dll export
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    include(GenerateExportHeader)
endif()




#set(Torch_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/thirdparty/libtorch/share/cmake/Torch")
#message("Torch_DIR ${Torch_DIR}")
#set(Caffe2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/thirdparty/libtorch/share/cmake/Caffe2")
#find_package(Torch PATHS ${Torch_DIR} NO_DEFAULT_PATH)
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

find_package(CUDA REQUIRED)

# ---------------------------------------------------------------------
# traverse source tree

#build cppgl
include_directories(external/advancedcppgl/src)
add_subdirectory(external/advancedcppgl)

include_directories(src)
add_subdirectory(src)





if (BUILD_ADVCPPGL_EXAMPLES)
    add_subdirectory(examples)
endif()
