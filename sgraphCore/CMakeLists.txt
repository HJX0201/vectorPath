add_library(smartCore STATIC
    s_application_settings_migration.cpp
    s_application_settings_migration.h
    s_result.h
)

target_include_directories(smartCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartCore PUBLIC Qt5::Core)
