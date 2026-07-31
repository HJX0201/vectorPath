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

add_executable(smartCamApplicationSettingsMigrationTests
    s_application_settings_migration_test.cpp
)

target_link_libraries(smartCamApplicationSettingsMigrationTests
    PRIVATE
        smartCore
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME smartCamApplicationSettingsMigrationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamApplicationSettingsMigrationTests>"
)

add_executable(smartCamDwgCodecTests
    s_dwg_codec_test.cpp
)

target_link_libraries(smartCamDwgCodecTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME smartCamDwgCodecTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamDwgCodecTests>"
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

add_executable(smartCamViewportTests
    s_cad_viewport_test.cpp
)

target_link_libraries(smartCamViewportTests
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
    NAME smartCamViewportTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamViewportTests>"
)

add_executable(smartCamCurveOffsetTests
    s_curve_offset_test.cpp
)

target_link_libraries(smartCamCurveOffsetTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamCurveOffsetTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamCurveOffsetTests>"
)

add_executable(smartCamArrayOperationsTests
    s_array_operations_test.cpp
)

target_link_libraries(smartCamArrayOperationsTests
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
    NAME smartCamArrayOperationsTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamArrayOperationsTests>"
)

add_executable(smartCamCurveFilletTests
    s_curve_fillet_test.cpp
)

target_link_libraries(smartCamCurveFilletTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamCurveFilletTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamCurveFilletTests>"
)

add_executable(smartCamSplineBlendTests
    s_spline_blend_test.cpp
)

target_link_libraries(smartCamSplineBlendTests
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
    NAME smartCamSplineBlendTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamSplineBlendTests>"
)

add_executable(smartCamSplineEditTests
    s_spline_edit_test.cpp
)

target_link_libraries(smartCamSplineEditTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamSplineEditTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamSplineEditTests>"
)

add_executable(smartCamAnnotationTests
    s_annotation_test.cpp
)

target_link_libraries(smartCamAnnotationTests
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
    NAME smartCamAnnotationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamAnnotationTests>"
)

add_executable(smartCamDimensionTests
    s_dimension_test.cpp
)

target_link_libraries(smartCamDimensionTests
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
    NAME smartCamDimensionTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamDimensionTests>"
)

add_executable(smartCamHatchTests
    s_hatch_test.cpp
)

target_link_libraries(smartCamHatchTests
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
    NAME smartCamHatchTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamHatchTests>"
)

add_executable(smartCamPolylineCornerTests
    s_polyline_corner_test.cpp
)

target_link_libraries(smartCamPolylineCornerTests
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
    NAME smartCamPolylineCornerTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamPolylineCornerTests>"
)

add_executable(smartCamGripEditTests
    s_grip_edit_test.cpp
)

target_link_libraries(smartCamGripEditTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamGripEditTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamGripEditTests>"
)

add_executable(smartCamPrecisionSnapTests
    s_precision_snap_test.cpp
)

target_link_libraries(smartCamPrecisionSnapTests
    PRIVATE
        smartRender
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME smartCamPrecisionSnapTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamPrecisionSnapTests>"
)

add_executable(smartCamCommandLineWidgetTests
    s_command_line_widget_test.cpp
)

target_link_libraries(smartCamCommandLineWidgetTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamCommandLineWidgetTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamCommandLineWidgetTests>"
)

add_executable(smartCamWindowControlTests
    s_window_control_test.cpp
)

target_link_libraries(smartCamWindowControlTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamWindowControlTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamWindowControlTests>"
)

add_executable(smartCamWorkspacePersistenceTests
    s_workspace_persistence_test.cpp
)

target_link_libraries(smartCamWorkspacePersistenceTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamWorkspacePersistenceTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamWorkspacePersistenceTests>"
)

add_executable(smartCamEllipseTests
    s_ellipse_test.cpp
)

target_link_libraries(smartCamEllipseTests
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
    NAME smartCamEllipseTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamEllipseTests>"
)

add_executable(smartCamCurveConstructionTests
    s_curve_construction_test.cpp
)

target_link_libraries(smartCamCurveConstructionTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamCurveConstructionTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamCurveConstructionTests>"
)

add_executable(smartCamShortcutManagerTests
    s_shortcut_manager_test.cpp
)

target_link_libraries(smartCamShortcutManagerTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamShortcutManagerTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamShortcutManagerTests>"
)

add_executable(smartCamDialogSizePolicyTests
    s_dialog_size_policy_test.cpp
)

target_link_libraries(smartCamDialogSizePolicyTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamDialogSizePolicyTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamDialogSizePolicyTests>"
)

add_executable(smartCamStandardShapeTests
    s_standard_shape_test.cpp
)

target_link_libraries(smartCamStandardShapeTests
    PRIVATE
        smartGeometry
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME smartCamStandardShapeTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamStandardShapeTests>"
)

add_executable(smartCamLayerColorWorkflowTests
    s_layer_color_workflow_test.cpp
)

target_link_libraries(smartCamLayerColorWorkflowTests
    PRIVATE
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME smartCamLayerColorWorkflowTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamLayerColorWorkflowTests>"
)

add_executable(smartCamPolygonBooleanTests
    s_polygon_boolean_test.cpp
)

target_link_libraries(smartCamPolygonBooleanTests
    PRIVATE
        smartGeometry
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME smartCamPolygonBooleanTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamPolygonBooleanTests>"
)

add_executable(smartCamUiScaleTests
    s_ui_scale_test.cpp
)

target_link_libraries(smartCamUiScaleTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamUiScaleTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamUiScaleTests>"
)

add_executable(smartCamToolpathSortTests
    s_toolpath_sort_test.cpp
)

target_link_libraries(smartCamToolpathSortTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME smartCamToolpathSortTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamToolpathSortTests>"
)

add_executable(smartCamToolpathSimulationTests
    s_toolpath_simulation_test.cpp
)

target_link_libraries(smartCamToolpathSimulationTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamToolpathSimulationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamToolpathSimulationTests>"
)

add_executable(smartCamQuickEntityOperationTests
    s_quick_entity_operation_test.cpp
)

target_link_libraries(smartCamQuickEntityOperationTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCamQuickEntityOperationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamQuickEntityOperationTests>"
)

add_executable(smartCamSvgVectorImportTests
    s_svg_vector_import_test.cpp
)

target_link_libraries(smartCamSvgVectorImportTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME smartCamSvgVectorImportTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCamSvgVectorImportTests>"
)
