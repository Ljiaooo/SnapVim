cmake_minimum_required(VERSION 3.21)

project(SnapVim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_SOURCES
    third_party/imgui/imgui.h
    third_party/imgui/imgui.cpp
    third_party/imgui/imconfig.h
    third_party/imgui/imgui_internal.h
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imstb_rectpack.h
    third_party/imgui/imstb_truetype.h
    third_party/imgui/imstb_textedit.h
    third_party/imgui/backend/imgui_impl_dx12.h
    third_party/imgui/backend/imgui_impl_dx12.cpp
    third_party/imgui/backend/imgui_impl_win32.h
    third_party/imgui/backend/imgui_impl_win32.cpp
)

set(SOURCES
    main.cpp
    resources.rc
    #imgui_demo.cpp
    #snapvim.cpp
    #snapvim.h
)

add_executable(SnapVim ${SOURCES})
target_sources(SnapVim PRIVATE ${IMGUI_SOURCES})
set_target_properties(SnapVim PROPERTIES WIN32_EXECUTABLE TRUE)

target_include_directories(SnapVim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backend)

target_link_libraries(SnapVim PRIVATE 
    d3d12
    d3dcompiler
    dxgi
    dwmapi
)