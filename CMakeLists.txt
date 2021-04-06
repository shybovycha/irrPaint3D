cmake_minimum_required(VERSION 3.16)

project(irr-paint-3d)

set(SOURCES "src/main.cpp")

add_executable(irr-paint-3d ${SOURCES})

find_package(irrlicht CONFIG REQUIRED)
target_link_libraries(irr-paint-3d PRIVATE Irrlicht)

# find_package(CImg CONFIG REQUIRED)
# target_link_libraries(irr-paint-3d PRIVATE CImg::CImg)

# copy media files to the target directory
add_custom_command(TARGET irr-paint-3d POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/media/dwarf.x ${CMAKE_CURRENT_BINARY_DIR}/dwarf.x)
add_custom_command(TARGET irr-paint-3d POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/media/dwarf.jpg ${CMAKE_CURRENT_BINARY_DIR}/dwarf.jpg)
