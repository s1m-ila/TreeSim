cmake_minimum_required(VERSION 3.16)
project(TreeSim LANGUAGES C CXX)

add_subdirectory(../smpb smpb)
add_subdirectory(../smgc smgc)
add_subdirectory(../sm-gui sm-gui)

# --- sm-gui::TreeSim ---
add_library(TreeSim_TreeSim STATIC
  src/TreeSim/Tree.cpp
  src/TreeSim/Map.cpp
  src/TreeSim/Scene2D.cpp
  src/TreeSim/WidgetMap.cpp
)
add_library(TreeSim::TreeSim ALIAS TreeSim_TreeSim)

target_include_directories(TreeSim_TreeSim
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ВАЖНО: заголовки TreeSim включают <smpb/...>, поэтому зависимость PUBLIC
target_link_libraries(TreeSim_TreeSim
  PUBLIC
    smpb::core
    smgc::display2D
    sm-gui::elements
)

set_target_properties(TreeSim_TreeSim PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(TreeSim_TreeSim PUBLIC cxx_std_23)

add_executable(TreeSim main.cpp)
target_link_libraries(TreeSim
  PRIVATE
    TreeSim::TreeSim
    smpb::core
    smgc::display2D
    sm-gui::elements
)

smgc_copy_files(TreeSim)

set(FILES "core.config")
foreach(FILE ${FILES})
    add_custom_command(
        TARGET TreeSim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/files/${FILE}"
        "$<TARGET_FILE_DIR:TreeSim>/files/${FILE}"
        COMMENT "Copying file ${FILE} to build directory..."
    )
endforeach()