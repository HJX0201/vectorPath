add_executable(smartGraphics WIN32
    s_main.cpp
    s_chinese_ui_translator.cpp
    s_chinese_ui_translator.h
)

target_link_libraries(smartGraphics PRIVATE smartGui Qt5::Core Qt5::Gui Qt5::Widgets)

set_target_properties(smartGraphics PROPERTIES
    OUTPUT_NAME smartGraphics
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:smartGraphics>"
)

if(WIN32)
    add_custom_command(TARGET smartGraphics POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:smartGraphics>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${Qt5_DIR}/../../../translations/qt_zh_CN.qm"
                "$<TARGET_FILE_DIR:smartGraphics>/translations/qt_zh_CN.qm"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${PROJECT_SOURCE_DIR}/sgraphThirdParty/libredwg-0.14-win64"
                "$<TARGET_FILE_DIR:smartGraphics>/libredwg"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:qtadvanceddocking-qt5>"
                "$<TARGET_FILE_DIR:smartGraphics>"
        COMMAND "${Qt5_DIR}/../../../bin/windeployqt.exe"
                --no-translations
                --no-system-d3d-compiler
                "$<TARGET_FILE:smartGraphics>"
        COMMENT "Deploying Qt runtime"
        VERBATIM
    )
endif()
