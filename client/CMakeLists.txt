cmake_minimum_required(VERSION 3.20)

project(Sarniensis LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk4 gtk4-layer-shell-0 epoxy)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
find_package(fmt REQUIRED)
set(Torch_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libtorch/share/cmake/Torch)
find_package(Torch REQUIRED)

foreach(_external_target PkgConfig::GTK PkgConfig::CURL fmt::fmt)
    get_target_property(_external_includes ${_external_target} INTERFACE_INCLUDE_DIRECTORIES)
    if(_external_includes)
        set_property(TARGET ${_external_target} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${_external_includes})
    endif()
endforeach()

add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
target_compile_options(imgui PRIVATE -include epoxy/gl.h -w)
target_include_directories(imgui SYSTEM PUBLIC third_party/imgui third_party/imgui/backends)
target_link_libraries(imgui PUBLIC PkgConfig::GTK)

add_library(rlsdk STATIC
    third_party/rlsdk/GameDefines.cpp
)

target_compile_features(rlsdk PRIVATE cxx_std_23)
target_compile_options(rlsdk PRIVATE -w)
target_include_directories(rlsdk SYSTEM PUBLIC
    third_party/rlsdk_compat
    third_party/rlsdk
)

file(GLOB_RECURSE ROCKETSIM_SOURCES CONFIGURE_DEPENDS
    third_party/RocketSim/src/*.cpp
    third_party/RocketSim/libsrc/bullet3-3.24/*.cpp
)

add_library(rocketsim STATIC ${ROCKETSIM_SOURCES})

target_compile_features(rocketsim PRIVATE cxx_std_20)
target_compile_options(rocketsim PRIVATE -O2 -w)
target_include_directories(rocketsim SYSTEM PUBLIC
    third_party/RocketSim/src
    third_party/RocketSim/libsrc/bullet3-3.24
)

file(GLOB_RECURSE SARNIENSIS_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(sarniensis ${SARNIENSIS_SOURCES})

add_executable(nerine_hidraw_gate tools/nerine_hidraw_gate.cpp)

add_executable(nerine_controller_setup tools/nerine_controller_setup.cpp)

target_compile_features(nerine_hidraw_gate PRIVATE cxx_std_23)
target_compile_options(nerine_hidraw_gate PRIVATE -Wall -Wextra -Wpedantic)
target_compile_features(nerine_controller_setup PRIVATE cxx_std_23)
target_compile_options(nerine_controller_setup PRIVATE -Wall -Wextra -Wpedantic)

add_custom_command(TARGET nerine_controller_setup POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/99-nerine_hidraw_gate.rules $<TARGET_FILE_DIR:nerine_controller_setup>/99-nerine_hidraw_gate.rules)
add_dependencies(sarniensis nerine_hidraw_gate nerine_controller_setup)

target_compile_features(sarniensis PRIVATE cxx_std_23)
target_compile_options(sarniensis PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(sarniensis PRIVATE
    ../include
    src
)
target_include_directories(sarniensis SYSTEM PRIVATE
    third_party/tomlplusplus
    third_party/nlohmann
    ${TORCH_INCLUDE_DIRS}
)
target_link_libraries(sarniensis PRIVATE imgui rlsdk rocketsim PkgConfig::GTK PkgConfig::CURL fmt::fmt ${TORCH_LIBRARIES})
set_target_properties(sarniensis PROPERTIES BUILD_RPATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libtorch/lib)
add_custom_command(TARGET sarniensis POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nexto/nexto-model.pt $<TARGET_FILE_DIR:sarniensis>/nexto-model.pt)
