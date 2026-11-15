cmake_minimum_required(VERSION 3.20)
project(WeatherPlus LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

set(PLUGIN_INTERFACE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/PluginInterface.h")
if(NOT EXISTS "${PLUGIN_INTERFACE_HEADER}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/zhongyang219/TrafficMonitor/master/TrafficMonitor/PluginInterface.h"
        "${PLUGIN_INTERFACE_HEADER}"
        TLS_VERIFY ON
    )
endif()

add_library(WeatherPlus SHARED
    src/GdiPlusInclude.h
    src/Config.h
    src/Config.cpp
    src/WeatherModels.h
    src/Localization.h
    src/WeatherClient.h
    src/WeatherClient.cpp
    src/IconRenderer.h
    src/IconRenderer.cpp
    src/OptionsDialog.h
    src/OptionsDialog.cpp
    src/WeatherItem.h
    src/WeatherItem.cpp
    src/WeatherPlugin.h
    src/WeatherPlugin.cpp
    src/resource.h
    src/WeatherPlugin.rc
)

target_include_directories(WeatherPlus PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(WeatherPlus PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
)

if(MSVC)
    target_compile_options(WeatherPlus PRIVATE /utf-8 /W3)
endif()

target_link_libraries(WeatherPlus PRIVATE
    nlohmann_json::nlohmann_json
    winhttp
    gdiplus
    comctl32
    user32
    gdi32
    msimg32
)