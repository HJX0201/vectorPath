cmake_minimum_required(VERSION 3.16)

project(smartGraphics VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
    # Ninja parses MSVC /showIncludes output to track header dependencies. Force the
    # compiler's English output so localized Windows installations cannot silently
    # leave stale object files after a class layout changes.
    set(CMAKE_CXX_COMPILER_LAUNCHER
        "${CMAKE_COMMAND};-E;env;VSLANG=1033"
    )
endif()

option(SMARTCAD_BUILD_TESTS "Build smartGraphics tests" ON)

set(SGRAPH_QT_COMPONENTS
    Core
    Gui
    Widgets
    OpenGL
    Svg
    PrintSupport
    Concurrent
)
if(SMARTCAD_BUILD_TESTS)
    list(APPEND SGRAPH_QT_COMPONENTS Test)
endif()
find_package(Qt5 5.12.10 REQUIRED COMPONENTS ${SGRAPH_QT_COMPONENTS})

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
endif()

set(SARIBBON_BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(SARIBBON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SARIBBON_USE_FRAMELESS_LIB OFF CACHE BOOL "" FORCE)
set(SARIBBON_INSTALL_IN_CURRENT_DIR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_Qt5LinguistTools ON)
add_subdirectory(sgraphThirdParty/SARibbon-2.9.0)
unset(CMAKE_DISABLE_FIND_PACKAGE_Qt5LinguistTools)

set(BUILD_STATIC OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ADS_VERSION 4.4.1 CACHE STRING "QtADS version" FORCE)
add_subdirectory(sgraphThirdParty/Qt-Advanced-Docking-System-4.4.1)

add_subdirectory(sgraphCore)
add_subdirectory(sgraphThirdParty/Clipper2)
add_subdirectory(sgraphGeometry)
add_subdirectory(sgraphDocument)
add_subdirectory(sgraphCommands)
add_subdirectory(sgraphIo)
add_subdirectory(sgraphRender)
add_subdirectory(sgraphGui)
add_subdirectory(sgraphApp)

if(SMARTCAD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(sgraphTests)
    add_subdirectory(sgraphVectorBenchmark)
endif()
