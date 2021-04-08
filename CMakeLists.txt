cmake_minimum_required(VERSION 3.16)

project(irr-paint-3d)

set(EXECUTABLE_NAME irr-paint-3d)
set(SOURCES "src/main.cpp" "src/Application.h" "src/Application.cpp" "src/IrrlichtEventReceiver.cpp" "src/ApplicationDelegate.h" "src/ApplicationDelegate.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCES})

find_package(irrlicht CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE Irrlicht)

# find_package(CImg CONFIG REQUIRED)
# target_link_libraries(irr-paint-3d PRIVATE CImg::CImg)

# copy media files to the target directory
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_LIST_DIR}/media $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/media)
