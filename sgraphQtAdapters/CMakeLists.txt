add_library(smartQtAdapters STATIC
    vp_qt_text.h
    vp_qt_geometry.h
    vp_qt_geometry.cpp
    vp_application_settings_migration.h
    vp_application_settings_migration.cpp
)
target_include_directories(smartQtAdapters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartQtAdapters PUBLIC smartCore smartGeometryCore Qt5::Core)
