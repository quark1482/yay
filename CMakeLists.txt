cmake_minimum_required(VERSION 3.5)

project(YAY VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebEngineCore)

set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/packages/ffmpeg_x64-windows)

include_directories(${FFMPEG_DIR}/include)

find_library(AVCODEC_LIBRARY NAMES avcodec HINTS ${FFMPEG_DIR}/lib)
find_library(AVFORMAT_LIBRARY NAMES avformat HINTS ${FFMPEG_DIR}/lib)
find_library(AVUTIL_LIBRARY NAMES avutil HINTS ${FFMPEG_DIR}/lib)

if(${AVCODEC_LIBRARY} STREQUAL "AVCODEC_LIBRARY-NOTFOUND")
    message(FATAL_ERROR "ffmpeg/avcodec library not found")
elseif(${AVFORMAT_LIBRARY} STREQUAL "AVFORMAT_LIBRARY-NOTFOUND")
    message(FATAL_ERROR "ffmpeg/avformat library not found")
elseif(${AVUTIL_LIBRARY} STREQUAL "AVUTIL_LIBRARY-NOTFOUND")
    message(FATAL_ERROR "ffmpeg/avutil library not found")
endif()

add_subdirectory(src)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(YAY
        MANUAL_FINALIZATION
        ${SRC_FILES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET YAY APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(YAY SHARED
            ${SRC_FILES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(YAY
            ${SRC_FILES}
        )
    endif()
endif()

target_link_libraries(YAY
    PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    PRIVATE Qt${QT_VERSION_MAJOR}::Network
    PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineCore
    PRIVATE ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY}
)

set_target_properties(YAY PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(YAY)
endif()
