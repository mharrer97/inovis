# target name to generate here
set(TARGET Inovis)

#list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../external/thirdparty/libtorch")

# glob source files
file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE SOURCES "*.cpp")

#also make exec dependant on cppgl and adv cppgl
file(GLOB_RECURSE HEADERS_ADV "../external/advancedcppgl/src/*.h")
file(GLOB_RECURSE SOURCES_ADV "../external/advancedcppgl/src/*.cpp")
file(GLOB_RECURSE HEADERS_CPPGL "../external/advancedcppgl/external/cppgl/src/*.h")
file(GLOB_RECURSE SOURCES_CPPGL "../external/advancedcppgl/external/cppgl/src/*.cpp")
file(GLOB_RECURSE HEADERS_UTIL "../util/*.h")
file(GLOB_RECURSE SOURCES_UTIL "../util/*.cpp")

# define target
add_executable(${TARGET} ${SOURCES} ${SOURCES_ADV} ${HEADERS} ${HEADERS_ADV} ${SOURCES_CPPGL} ${HEADERS_CPPGL} ${SOURCES_UTIL} ${HEADERS_UTIL} frustum.h)


# forces executables to be compiled to /example/ folder, to allow relative paths for shaders
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
# ----------------------------------------------------------
# dependencies

target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../external/advancedcppgl/src")

# glm
target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../external/advancedcppgl/external/cppgl/external/thirdparty")

#message(${CONDA_PATH})
target_include_directories(${TARGET} PUBLIC "${CONDA_PATH}/include")
#target_include_directories(${TARGET} PUBLIC "~/anaconda3/envs/inovis/include")
target_link_libraries(${TARGET} ${CUDA_LIBRARIES})

set(DEPDIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/thirdparty")
target_include_directories(${TARGET} PUBLIC ${DEPDIR}/include)

target_link_libraries(${TARGET} "${TORCH_LIBRARIES}")

if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  #message(">>>>>>>>>>>>>>>")
  #message(${TORCH_INSTALL_PREFIX}/lib/*.dll)
  add_custom_command(TARGET ${TARGET}
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     ${TORCH_DLLS}
                     $<TARGET_FILE_DIR:${TARGET}>)
endif (MSVC)

if(UNIX)
  
  # std::filesystem
	target_link_libraries(${TARGET} stdc++fs)
  
  #find_library(ASSIMP_LIB libassimp)
  #target_link_libraries()
  set(LIN_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../external/advancedcppl/external/cppgl/external/thirdparty/lin/lib")
  #message(("UNIX LIB"))
  #message("${LIN_LIB_PATH}")
  #target_link_directories(${TARGET} PUBLIC "${LIN_LIB_PATH}")
  #target_link_libraries(${TARGET} "${LIN_LIB_PATH}")
  #target_link_libraries(${TARGET} "libassimp" "libGLEW" "libGLFW")
  find_library(GLEW GLEW PATHS ${LIN_LIB_PATH} NO_DEFAULT_PATH)
  find_library(GLFW glfw PATHS ${LIN_LIB_PATH} NO_DEFAULT_PATH)
  find_library(ASSIMP assimp PATHS ${LIN_LIB_PATH} NO_DEFAULT_PATH)
  install(DIRECTORY ${LIN_LIB_PATH} DESTINATION .) # install deps
  # ${ASSIMP}
  target_link_libraries(${TARGET} ${GLEW} ${GLFW})
else()

	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../external/advancedcppgl/external/cppgl/external/thirdparty/win/bin/x64" $<TARGET_FILE_DIR:${TARGET}>)
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/external/advancedcppgl/advancedcppgl.dll" $<TARGET_FILE_DIR:${TARGET}>)
	#add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../external/thirdparty/win/bin/x64" $<TARGET_FILE_DIR:${TARGET}>)
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/external/advancedcppgl/external/cppgl/cppgl.dll" $<TARGET_FILE_DIR:${TARGET}>)
endif()

# built libs
target_link_libraries(${TARGET} cppgl)
target_link_libraries(${TARGET} advancedcppgl)


