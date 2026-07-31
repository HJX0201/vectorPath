add_executable(smartDocumentTests
    s_cad_document_test.cpp
)

target_link_libraries(smartDocumentTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME smartDocumentTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartDocumentTests>"
)

add_executable(vectorPathApplicationSettingsMigrationTests
    s_application_settings_migration_test.cpp
)

target_link_libraries(vectorPathApplicationSettingsMigrationTests
    PRIVATE
        smartCore
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME vectorPathApplicationSettingsMigrationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathApplicationSettingsMigrationTests>"
)

add_executable(vectorPathDwgCodecTests
    s_dwg_codec_test.cpp
)

target_link_libraries(vectorPathDwgCodecTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME vectorPathDwgCodecTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathDwgCodecTests>"
)

add_executable(smartDocumentRecoveryTests
    s_document_recovery_test.cpp
)

target_link_libraries(smartDocumentRecoveryTests
    PRIVATE
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME smartDocumentRecoveryTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartDocumentRecoveryTests>"
)

add_executable(smartDocumentAuditTests
    s_document_audit_test.cpp
)

target_link_libraries(smartDocumentAuditTests
    PRIVATE
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME smartDocumentAuditTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartDocumentAuditTests>"
)

add_executable(vectorPathViewportTests
    s_cad_viewport_test.cpp
)

target_link_libraries(vectorPathViewportTests
    PRIVATE
        smartRender
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathViewportTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathViewportTests>"
)

add_executable(vectorPathCurveOffsetTests
    s_curve_offset_test.cpp
)

target_link_libraries(vectorPathCurveOffsetTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathCurveOffsetTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathCurveOffsetTests>"
)

add_executable(vectorPathArrayOperationsTests
    s_array_operations_test.cpp
)

target_link_libraries(vectorPathArrayOperationsTests
    PRIVATE
        smartRender
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathArrayOperationsTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathArrayOperationsTests>"
)

add_executable(vectorPathCurveFilletTests
    s_curve_fillet_test.cpp
)

target_link_libraries(vectorPathCurveFilletTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathCurveFilletTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathCurveFilletTests>"
)

add_executable(vectorPathSplineBlendTests
    s_spline_blend_test.cpp
)

target_link_libraries(vectorPathSplineBlendTests
    PRIVATE
        smartRender
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathSplineBlendTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathSplineBlendTests>"
)

add_executable(vectorPathSplineEditTests
    s_spline_edit_test.cpp
)

target_link_libraries(vectorPathSplineEditTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathSplineEditTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathSplineEditTests>"
)

add_executable(vectorPathAnnotationTests
    s_annotation_test.cpp
)

target_link_libraries(vectorPathAnnotationTests
    PRIVATE
        smartRender
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathAnnotationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathAnnotationTests>"
)

add_executable(vectorPathDimensionTests
    s_dimension_test.cpp
)

target_link_libraries(vectorPathDimensionTests
    PRIVATE
        smartRender
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathDimensionTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathDimensionTests>"
)

add_executable(vectorPathHatchTests
    s_hatch_test.cpp
)

target_link_libraries(vectorPathHatchTests
    PRIVATE
        smartRender
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathHatchTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathHatchTests>"
)

add_executable(vectorPathPolylineCornerTests
    s_polyline_corner_test.cpp
)

target_link_libraries(vectorPathPolylineCornerTests
    PRIVATE
        smartRender
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathPolylineCornerTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathPolylineCornerTests>"
)

add_executable(vectorPathGripEditTests
    s_grip_edit_test.cpp
)

target_link_libraries(vectorPathGripEditTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathGripEditTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathGripEditTests>"
)

add_executable(vectorPathPrecisionSnapTests
    s_precision_snap_test.cpp
)

target_link_libraries(vectorPathPrecisionSnapTests
    PRIVATE
        smartRender
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME vectorPathPrecisionSnapTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathPrecisionSnapTests>"
)

add_executable(vectorPathCommandLineWidgetTests
    s_command_line_widget_test.cpp
)

target_link_libraries(vectorPathCommandLineWidgetTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathCommandLineWidgetTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathCommandLineWidgetTests>"
)

add_executable(vectorPathWindowControlTests
    s_window_control_test.cpp
)

target_link_libraries(vectorPathWindowControlTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathWindowControlTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathWindowControlTests>"
)

add_executable(vectorPathWorkspacePersistenceTests
    s_workspace_persistence_test.cpp
)

target_link_libraries(vectorPathWorkspacePersistenceTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathWorkspacePersistenceTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathWorkspacePersistenceTests>"
)

add_executable(vectorPathEllipseTests
    s_ellipse_test.cpp
)

target_link_libraries(vectorPathEllipseTests
    PRIVATE
        smartRender
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathEllipseTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathEllipseTests>"
)

add_executable(vectorPathCurveConstructionTests
    s_curve_construction_test.cpp
)

target_link_libraries(vectorPathCurveConstructionTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathCurveConstructionTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathCurveConstructionTests>"
)

add_executable(vectorPathShortcutManagerTests
    s_shortcut_manager_test.cpp
)

target_link_libraries(vectorPathShortcutManagerTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathShortcutManagerTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathShortcutManagerTests>"
)

add_executable(vectorPathDialogSizePolicyTests
    s_dialog_size_policy_test.cpp
)

target_link_libraries(vectorPathDialogSizePolicyTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathDialogSizePolicyTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathDialogSizePolicyTests>"
)

add_executable(vectorPathStandardShapeTests
    s_standard_shape_test.cpp
)

target_link_libraries(vectorPathStandardShapeTests
    PRIVATE
        smartGeometry
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME vectorPathStandardShapeTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathStandardShapeTests>"
)

add_executable(vectorPathLayerColorWorkflowTests
    s_layer_color_workflow_test.cpp
)

target_link_libraries(vectorPathLayerColorWorkflowTests
    PRIVATE
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME vectorPathLayerColorWorkflowTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathLayerColorWorkflowTests>"
)

add_executable(vectorPathPolygonBooleanTests
    s_polygon_boolean_test.cpp
)

target_link_libraries(vectorPathPolygonBooleanTests
    PRIVATE
        smartGeometry
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME vectorPathPolygonBooleanTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathPolygonBooleanTests>"
)

add_executable(vectorPathUiScaleTests
    s_ui_scale_test.cpp
)

target_link_libraries(vectorPathUiScaleTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathUiScaleTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathUiScaleTests>"
)

add_executable(vectorPathToolpathSortTests
    s_toolpath_sort_test.cpp
)

target_link_libraries(vectorPathToolpathSortTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME vectorPathToolpathSortTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathToolpathSortTests>"
)

add_executable(vectorPathToolpathSimulationTests
    s_toolpath_simulation_test.cpp
)

target_link_libraries(vectorPathToolpathSimulationTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathToolpathSimulationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathToolpathSimulationTests>"
)

add_executable(vectorPathQuickEntityOperationTests
    s_quick_entity_operation_test.cpp
)

target_link_libraries(vectorPathQuickEntityOperationTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME vectorPathQuickEntityOperationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathQuickEntityOperationTests>"
)

add_executable(vectorPathSvgVectorImportTests
    s_svg_vector_import_test.cpp
)

target_link_libraries(vectorPathSvgVectorImportTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME vectorPathSvgVectorImportTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathSvgVectorImportTests>"
)
