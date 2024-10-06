cmake_minimum_required(VERSION 3.25) 
project(Goliath CXX)               
set(CMAKE_CXX_STANDARD 17)            
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include("${CMAKE_SOURCE_DIR}/../../addons/prism/cmake/prism.cmake" REQUIRED)

# Add source/header files to project
file(GLOB_RECURSE SOURCES ../*.cpp SOURCES ../*.h SOURCES ../*.rc)
list(FILTER SOURCES EXCLUDE REGEX ".*web.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/build/.*")

add_link_options(/NODEFAULTLIB:libcmt.lib)
add_link_options(/IGNORE:4099,4286,4098)
add_link_options(/OPT:REF)
add_link_options(/OPT:ICF)

# Define exe
add_executable(Goliath ${SOURCES})

# Link with libraries
target_link_libraries(Goliath prism)
target_link_libraries(Goliath SDL2)
target_link_libraries(Goliath SDL2main)
target_link_libraries(Goliath SDL2_image)
target_link_libraries(Goliath SDL2_mixer)
target_link_libraries(Goliath SDL2_ttf)
target_link_libraries(Goliath enet)
target_link_libraries(Goliath freetype)
target_link_libraries(Goliath glew)
target_link_libraries(Goliath curl)
target_link_libraries(Goliath libjpeg)
target_link_libraries(Goliath libpng)
target_link_libraries(Goliath libwebp)
target_link_libraries(Goliath zstd)
target_link_libraries(Goliath zlib)
target_link_libraries(Goliath crypt32)
target_link_libraries(Goliath wldap32)
target_link_libraries(Goliath ws2_32)
target_link_libraries(Goliath winmm)
target_link_libraries(Goliath OpenGL32)
target_link_libraries(Goliath version)

# Define preprocessor definitions
target_compile_definitions(Goliath PUBLIC UNICODE)
target_compile_definitions(Goliath PUBLIC _UNICODE)

set_property(TARGET Goliath PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(Goliath PRIVATE /Gy)

set_target_properties(Goliath PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../assets)
set_target_properties(Goliath PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/../assets)

# Copy over DLLs
file(GLOB DLLS "${CMAKE_SOURCE_DIR}/../../addons/prism/windows/vs17/DLL/*.dll")
foreach(DLL ${DLLS})
  add_custom_command(TARGET Goliath POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${DLL}
    $<TARGET_FILE_DIR:Goliath>)
endforeach()

# Optimize for release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(Goliath PRIVATE /O2)
endif()
