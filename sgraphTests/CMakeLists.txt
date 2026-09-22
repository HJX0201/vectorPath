add_executable(vectorPathCoreResultTests vp_core_result_test.cpp)
target_link_libraries(vectorPathCoreResultTests PRIVATE smartCore)
add_test(NAME vectorPathCoreResultTests COMMAND vectorPathCoreResultTests)

add_executable(vectorPathGeometryCoreTests vp_geometry_core_test.cpp)
target_link_libraries(vectorPathGeometryCoreTests PRIVATE smartGeometryCore)
add_test(NAME vectorPathGeometryCoreTests COMMAND vectorPathGeometryCoreTests)

add_executable(vectorPathIdCollectionTests vp_id_collection_test.cpp)
target_link_libraries(vectorPathIdCollectionTests PRIVATE smartCore)
add_test(NAME vectorPathIdCollectionTests COMMAND vectorPathIdCollectionTests)

# Opt-in measurement: timing is intentionally not a pass/fail CTest threshold.
add_executable(vectorPathIdCollectionBenchmark EXCLUDE_FROM_ALL vp_id_collection_benchmark.cpp)
target_link_libraries(vectorPathIdCollectionBenchmark PRIVATE smartCore)

add_executable(vectorPathDraftingStateTests vp_drafting_state_test.cpp)
target_link_libraries(vectorPathDraftingStateTests PRIVATE smartInteraction)
add_test(NAME vectorPathDraftingStateTests COMMAND vectorPathDraftingStateTests)

if(NOT VECTORPATH_BUILD_DESKTOP)
    return()
endif()

add_executable(vectorPathQtAdapterTests vp_qt_adapter_test.cpp)
target_link_libraries(vectorPathQtAdapterTests PRIVATE smartQtAdapters Qt5::Test)
add_test(NAME vectorPathQtAdapterTests COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$ENV{PATH}"
    "$<TARGET_FILE:vectorPathQtAdapterTests>")

add_executable(vectorPathDocumentHistoryTests vp_document_history_test.cpp)
target_link_libraries(vectorPathDocumentHistoryTests PRIVATE smartDocument Qt5::Test)
add_test(NAME vectorPathDocumentHistoryTests COMMAND ${CMAKE_COMMAND} -E env
    "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$ENV{PATH}"
    "$<TARGET_FILE:vectorPathDocumentHistoryTests>")

add_executable(smartDocumentTests
    vp_cad_document_test.cpp
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
    vp_application_settings_migration_test.cpp
)

target_link_libraries(vectorPathApplicationSettingsMigrationTests
    PRIVATE
        smartCore smartQtAdapters
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
    vp_dwg_codec_test.cpp
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
            "VECTORPATH_LIBREDWG_DIR=${PROJECT_SOURCE_DIR}/sgraphThirdParty/libredwg-0.14-win64"
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
            "$<TARGET_FILE:vectorPathDwgCodecTests>"
)

add_executable(smartDocumentRecoveryTests
    vp_document_recovery_test.cpp
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
    vp_document_audit_test.cpp
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
    vp_cad_viewport_test.cpp
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
    vp_curve_offset_test.cpp
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
    vp_array_operations_test.cpp
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
    vp_curve_fillet_test.cpp
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
    vp_spline_blend_test.cpp
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
    vp_spline_edit_test.cpp
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
    vp_annotation_test.cpp
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
    vp_dimension_test.cpp
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
    vp_hatch_test.cpp
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
    vp_polyline_corner_test.cpp
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
    vp_grip_edit_test.cpp
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
    vp_precision_snap_test.cpp
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
    vp_command_line_widget_test.cpp
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
    vp_window_control_test.cpp
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
    vp_workspace_persistence_test.cpp
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
    vp_ellipse_test.cpp
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
    vp_curve_construction_test.cpp
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
    vp_shortcut_manager_test.cpp
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
    vp_dialog_size_policy_test.cpp
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
    vp_standard_shape_test.cpp
)

target_link_libraries(vectorPathStandardShapeTests
    PRIVATE
        smartGeometry smartQtAdapters
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
    vp_layer_color_workflow_test.cpp
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
    vp_polygon_boolean_test.cpp
)

target_link_libraries(vectorPathPolygonBooleanTests
    PRIVATE
        smartGeometry smartQtAdapters
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
    vp_ui_scale_test.cpp
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
    vp_toolpath_sort_test.cpp
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
    vp_toolpath_simulation_test.cpp
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
    vp_quick_entity_operation_test.cpp
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
    vp_svg_vector_import_test.cpp
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
