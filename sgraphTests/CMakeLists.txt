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

add_executable(smartCadDwgCodecTests
    s_dwg_codec_test.cpp
)

target_link_libraries(smartCadDwgCodecTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME smartCadDwgCodecTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadDwgCodecTests>"
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

add_executable(smartCadViewportTests
    s_cad_viewport_test.cpp
)

target_link_libraries(smartCadViewportTests
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
    NAME smartCadViewportTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadViewportTests>"
)

add_executable(smartCadCurveOffsetTests
    s_curve_offset_test.cpp
)

target_link_libraries(smartCadCurveOffsetTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadCurveOffsetTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadCurveOffsetTests>"
)

add_executable(smartCadArrayOperationsTests
    s_array_operations_test.cpp
)

target_link_libraries(smartCadArrayOperationsTests
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
    NAME smartCadArrayOperationsTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadArrayOperationsTests>"
)

add_executable(smartCadCurveFilletTests
    s_curve_fillet_test.cpp
)

target_link_libraries(smartCadCurveFilletTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadCurveFilletTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadCurveFilletTests>"
)

add_executable(smartCadSplineBlendTests
    s_spline_blend_test.cpp
)

target_link_libraries(smartCadSplineBlendTests
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
    NAME smartCadSplineBlendTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadSplineBlendTests>"
)

add_executable(smartCadSplineEditTests
    s_spline_edit_test.cpp
)

target_link_libraries(smartCadSplineEditTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadSplineEditTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadSplineEditTests>"
)

add_executable(smartCadAnnotationTests
    s_annotation_test.cpp
)

target_link_libraries(smartCadAnnotationTests
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
    NAME smartCadAnnotationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadAnnotationTests>"
)

add_executable(smartCadDimensionTests
    s_dimension_test.cpp
)

target_link_libraries(smartCadDimensionTests
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
    NAME smartCadDimensionTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadDimensionTests>"
)

add_executable(smartCadHatchTests
    s_hatch_test.cpp
)

target_link_libraries(smartCadHatchTests
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
    NAME smartCadHatchTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadHatchTests>"
)

add_executable(smartCadPolylineCornerTests
    s_polyline_corner_test.cpp
)

target_link_libraries(smartCadPolylineCornerTests
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
    NAME smartCadPolylineCornerTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadPolylineCornerTests>"
)

add_executable(smartCadGripEditTests
    s_grip_edit_test.cpp
)

target_link_libraries(smartCadGripEditTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadGripEditTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadGripEditTests>"
)

add_executable(smartCadPrecisionSnapTests
    s_precision_snap_test.cpp
)

target_link_libraries(smartCadPrecisionSnapTests
    PRIVATE
        smartRender
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME smartCadPrecisionSnapTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadPrecisionSnapTests>"
)

add_executable(smartCadCommandLineWidgetTests
    s_command_line_widget_test.cpp
)

target_link_libraries(smartCadCommandLineWidgetTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadCommandLineWidgetTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadCommandLineWidgetTests>"
)

add_executable(smartCadWindowControlTests
    s_window_control_test.cpp
)

target_link_libraries(smartCadWindowControlTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadWindowControlTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadWindowControlTests>"
)

add_executable(smartCadWorkspacePersistenceTests
    s_workspace_persistence_test.cpp
)

target_link_libraries(smartCadWorkspacePersistenceTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadWorkspacePersistenceTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadWorkspacePersistenceTests>"
)

add_executable(smartCadEllipseTests
    s_ellipse_test.cpp
)

target_link_libraries(smartCadEllipseTests
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
    NAME smartCadEllipseTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadEllipseTests>"
)

add_executable(smartCadCurveConstructionTests
    s_curve_construction_test.cpp
)

target_link_libraries(smartCadCurveConstructionTests
    PRIVATE
        smartRender
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadCurveConstructionTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadCurveConstructionTests>"
)

add_executable(smartCadShortcutManagerTests
    s_shortcut_manager_test.cpp
)

target_link_libraries(smartCadShortcutManagerTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadShortcutManagerTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadShortcutManagerTests>"
)

add_executable(smartCadDialogSizePolicyTests
    s_dialog_size_policy_test.cpp
)

target_link_libraries(smartCadDialogSizePolicyTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadDialogSizePolicyTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadDialogSizePolicyTests>"
)

add_executable(smartCadStandardShapeTests
    s_standard_shape_test.cpp
)

target_link_libraries(smartCadStandardShapeTests
    PRIVATE
        smartGeometry
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME smartCadStandardShapeTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadStandardShapeTests>"
)

add_executable(smartCadLayerColorWorkflowTests
    s_layer_color_workflow_test.cpp
)

target_link_libraries(smartCadLayerColorWorkflowTests
    PRIVATE
        smartDocument
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME smartCadLayerColorWorkflowTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadLayerColorWorkflowTests>"
)

add_executable(smartCadPolygonBooleanTests
    s_polygon_boolean_test.cpp
)

target_link_libraries(smartCadPolygonBooleanTests
    PRIVATE
        smartGeometry
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME smartCadPolygonBooleanTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadPolygonBooleanTests>"
)

add_executable(smartCadUiScaleTests
    s_ui_scale_test.cpp
)

target_link_libraries(smartCadUiScaleTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadUiScaleTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadUiScaleTests>"
)

add_executable(smartCadToolpathSortTests
    s_toolpath_sort_test.cpp
)

target_link_libraries(smartCadToolpathSortTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Test
)

add_test(
    NAME smartCadToolpathSortTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadToolpathSortTests>"
)

add_executable(smartCadToolpathSimulationTests
    s_toolpath_simulation_test.cpp
)

target_link_libraries(smartCadToolpathSimulationTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadToolpathSimulationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadToolpathSimulationTests>"
)

add_executable(smartCadQuickEntityOperationTests
    s_quick_entity_operation_test.cpp
)

target_link_libraries(smartCadQuickEntityOperationTests
    PRIVATE
        smartGui
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
)

add_test(
    NAME smartCadQuickEntityOperationTests
    COMMAND ${CMAKE_COMMAND} -E env
            "QT_QPA_PLATFORM=offscreen"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadQuickEntityOperationTests>"
)

add_executable(smartCadSvgVectorImportTests
    s_svg_vector_import_test.cpp
)

target_link_libraries(smartCadSvgVectorImportTests
    PRIVATE
        smartDocument
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Test
)

add_test(
    NAME smartCadSvgVectorImportTests
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:smartCadSvgVectorImportTests>"
)
