# target name to generate here
set(TARGET advancedcppgl)

# glob source files
file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE SOURCES "*.cpp")

# define target
add_library(${TARGET} SHARED ${SOURCES} ${HEADERS})
generate_export_header(${TARGET})
set_target_properties(${TARGET} PROPERTIES VERSION 0.1 DESCRIPTION "xx")

# install
install(TARGETS ${TARGET} DESTINATION lib)
install(DIRECTORY . DESTINATION include/${TARGET} FILES_MATCHING PATTERN "*.h")

# ----------------------------------------------------------
# dependencies

set(DEPDIR_ADV "${CMAKE_CURRENT_SOURCE_DIR}/../external/thirdparty")
target_include_directories(${TARGET} PUBLIC ${DEPDIR_ADV}/include)

#inherite all dependencies from cppgl
set(DEPDIR_CPPGL "${CMAKE_CURRENT_SOURCE_DIR}/../external/cppgl/external/thirdparty")
target_include_directories(${TARGET} PUBLIC ${DEPDIR_CPPGL}/include)

# opengl
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
target_include_directories(${TARGET} PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(${TARGET} PUBLIC ${OPENGL_LIBRARIES})

#openmp
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${TARGET} PUBLIC OpenMP::OpenMP_CXX)
endif()

if(UNIX)
# find_library(GLEW GLEW PATHS ${DEPDIR_CPPGL}/lin/lib NO_DEFAULT_PATH)
# find_library(GLFW glfw PATHS ${DEPDIR_CPPGL}/lin/lib NO_DEFAULT_PATH)
#	find_library(ASSIMP assimp PATHS ${DEPDIR_CPPGL}/lin/lib NO_DEFAULT_PATH)
  target_link_libraries(${TARGET} PUBLIC stdc++fs) # required for std::filesystem
else()
#	find_library(GLEW glew32 PATHS ${DEPDIR_CPPGL}/win/lib NO_DEFAULT_PATH)
# find_library(GLFW glfw3 PATHS ${DEPDIR_CPPGL}/win/lib NO_DEFAULT_PATH)
#	find_library(ASSIMP assimp PATHS ${DEPDIR_CPPGL}/win/lib NO_DEFAULT_PATH)
  target_compile_definitions(${TARGET} PUBLIC -DBUILD_ADVANCED_CPPGL_DLL)


endif()

#set standard shader path
target_compile_definitions(${TARGET} PUBLIC -DCOMPILED_STANDARD_SHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/standardShaders")

target_link_libraries(${TARGET} PUBLIC cppgl)

#target_link_libraries(${TARGET} ${GLEW} ${GLFW} ${ASSIMP})
