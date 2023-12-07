# target name to generate here
set(TARGET cppgl)

# glob source files
file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE SOURCES "*.cpp")

# define target
add_library(${TARGET} SHARED ${SOURCES} ${HEADERS})
generate_export_header(${TARGET})
set_target_properties(${TARGET} PROPERTIES VERSION 1.0 DESCRIPTION "xx")

# install
install(TARGETS ${TARGET} DESTINATION lib)
install(DIRECTORY . DESTINATION include/${TARGET} FILES_MATCHING PATTERN "*.h")

#public interface (cppgl.h is an available header file for all dependant projects)
target_include_directories(${TARGET} INTERFACE .)

# ----------------------------------------------------------
# dependencies

set(DEPDIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/thirdparty")
target_include_directories(${TARGET} PUBLIC ${DEPDIR}/include)

# opengl
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
target_include_directories(${TARGET} PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(${TARGET} ${OPENGL_LIBRARIES})

if(UNIX)
    find_library(GLEW GLEW PATHS ${DEPDIR}/lin/lib NO_DEFAULT_PATH)
    find_library(GLFW glfw PATHS ${DEPDIR}/lin/lib NO_DEFAULT_PATH)
    find_library(ASSIMP assimp PATHS ${DEPDIR}/lin/lib NO_DEFAULT_PATH)
    target_link_libraries(${TARGET} stdc++fs) # required for std::filesystem
    install(DIRECTORY ${DEPDIR}/lin/lib DESTINATION .) # install deps
    target_link_libraries(${TARGET} ${GLEW} ${GLFW} ${ASSIMP})
else()
	find_library(GLEW glew32 PATHS ${DEPDIR}/win/lib NO_DEFAULT_PATH)
    find_library(GLFW glfw3 PATHS ${DEPDIR}/win/lib NO_DEFAULT_PATH)
	find_library(ASSIMP assimp PATHS ${DEPDIR}/win/lib NO_DEFAULT_PATH)
    find_library(ASSIMPD assimpd PATHS ${DEPDIR}/win/lib NO_DEFAULT_PATH)
    target_compile_definitions(${TARGET} PRIVATE -DBUILD_CPPGL_DLL)
    target_compile_definitions(${TARGET} PRIVATE -DBUILD_SHARED_LIBS)
    target_link_libraries(${TARGET} ${GLEW} ${GLFW})
    target_link_libraries(${TARGET} optimized ${ASSIMP} debug ${ASSIMPD})
endif()


