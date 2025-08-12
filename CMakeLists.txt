cmake_minimum_required(VERSION 3.17)

project(SnapVim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/libvim)

set(IMGUI_SOURCES
    imgui/imgui.h
    imgui/imgui.cpp
    imgui/imconfig.h
    imgui/imgui_internal.h
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imstb_rectpack.h
    imgui/imstb_truetype.h
    imgui/imstb_textedit.h
    imgui/backend/imgui_impl_dx12.h
    imgui/backend/imgui_impl_dx12.cpp
    imgui/backend/imgui_impl_win32.h
    imgui/backend/imgui_impl_win32.cpp
)

set(SNAPVIM_SOURCES
    main.cpp
    res/resources.rc
    snapvim.cpp
    snapvim.h
)

add_executable(SnapVim ${SNAPVIM_SOURCES})
target_sources(SnapVim PRIVATE ${IMGUI_SOURCES})
set_target_properties(SnapVim PROPERTIES WIN32_EXECUTABLE TRUE)

target_include_directories(SnapVim PRIVATE
    ${CMAKE_SOURCE_DIR}
    imgui
    imgui/backend
    libvim/prebuilt/include
    libvim/prebuilt/include/proto)

target_link_libraries(SnapVim PRIVATE 
    d3d12
    d3dcompiler
    dxgi
    dwmapi
    netapi32
    libvim_static
)