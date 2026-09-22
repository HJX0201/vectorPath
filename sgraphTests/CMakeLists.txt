if(VECTORPATH_BUILD_BENCHMARKS)
    add_executable(vectorPathIdCollectionBenchmark vp_id_collection_benchmark.cpp)
    target_link_libraries(vectorPathIdCollectionBenchmark PRIVATE smartCore)
endif()
if(NOT VECTORPATH_BUILD_TESTS)
    return()
endif()

# Each suite still runs in its own process; only the build entry points are shared.
set(VP_CORE_SUITES
    "vectorPathCoreResultTests|vp_core_result_test.cpp|vpRunCoreResultTests"
    "vectorPathGeometryCoreTests|vp_geometry_core_test.cpp|vpRunGeometryCoreTests"
    "vectorPathIdCollectionTests|vp_id_collection_test.cpp|vpRunIdCollectionTests"
    "vectorPathDraftingStateTests|vp_drafting_state_test.cpp|vpRunDraftingStateTests"
)
add_executable(vectorPathCoreTests vp_core_test_main.cpp)
target_link_libraries(vectorPathCoreTests PRIVATE smartGeometryCore)
target_include_directories(vectorPathCoreTests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(vectorPathCoreTests PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
set(vp_core_registry "")
foreach(vp_suite IN LISTS VP_CORE_SUITES)
    string(REPLACE "|" ";" vp_parts "${vp_suite}")
    list(GET vp_parts 0 vp_name)
    list(GET vp_parts 1 vp_source)
    list(GET vp_parts 2 vp_entry)
    target_sources(vectorPathCoreTests PRIVATE ${vp_source})
    string(APPEND vp_core_registry "VECTORPATH_CORE_TEST(${vp_name}, ${vp_entry})\n")
    add_test(NAME ${vp_name} COMMAND vectorPathCoreTests ${vp_name})
    set_tests_properties(${vp_name} PROPERTIES LABELS "core")
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/vp_core_test_registry.inc" "${vp_core_registry}")

if(NOT VECTORPATH_BUILD_DESKTOP)
    return()
endif()
set(VP_DESKTOP_SUITES
    "vectorPathQtAdapterTests|vp_qt_adapter_test.cpp|vpRunVpQtAdapterTest|AppLess"
    "vectorPathDocumentHistoryTests|vp_document_history_test.cpp|vpRunVpDocumentHistoryTest|Core"
    "smartDocumentTests|vp_cad_document_test.cpp|vpRunVpCadDocumentTest|AppLess"
    "vectorPathApplicationSettingsMigrationTests|vp_application_settings_migration_test.cpp|vpRunVpApplicationSettingsMigrationTest|AppLess"
    "vectorPathDwgCodecTests|vp_dwg_codec_test.cpp|vpRunVpDwgCodecTest|Gui"
    "smartDocumentRecoveryTests|vp_document_recovery_test.cpp|vpRunVpDocumentRecoveryTest|Gui"
    "smartDocumentAuditTests|vp_document_audit_test.cpp|vpRunVpDocumentAuditTest|Gui"
    "vectorPathViewportTests|vp_cad_viewport_test.cpp|vpRunVpCadViewportTest|Widgets"
    "vectorPathCurveOffsetTests|vp_curve_offset_test.cpp|vpRunVpCurveOffsetTest|Widgets"
    "vectorPathArrayOperationsTests|vp_array_operations_test.cpp|vpRunVpArrayOperationsTest|Widgets"
    "vectorPathCurveFilletTests|vp_curve_fillet_test.cpp|vpRunVpCurveFilletTest|Widgets"
    "vectorPathSplineBlendTests|vp_spline_blend_test.cpp|vpRunVpSplineBlendTest|Widgets"
    "vectorPathSplineEditTests|vp_spline_edit_test.cpp|vpRunVpSplineEditTest|Widgets"
    "vectorPathAnnotationTests|vp_annotation_test.cpp|vpRunVpAnnotationTest|Widgets"
    "vectorPathDimensionTests|vp_dimension_test.cpp|vpRunVpDimensionTest|Widgets"
    "vectorPathHatchTests|vp_hatch_test.cpp|vpRunVpHatchTest|Widgets"
    "vectorPathPolylineCornerTests|vp_polyline_corner_test.cpp|vpRunVpPolylineCornerTest|Widgets"
    "vectorPathGripEditTests|vp_grip_edit_test.cpp|vpRunVpGripEditTest|Widgets"
    "vectorPathPrecisionSnapTests|vp_precision_snap_test.cpp|vpRunVpPrecisionSnapTest|Widgets"
    "vectorPathCommandLineWidgetTests|vp_command_line_widget_test.cpp|vpRunVpCommandLineWidgetTest|Widgets"
    "vectorPathWindowControlTests|vp_window_control_test.cpp|vpRunVpWindowControlTest|Widgets"
    "vectorPathWorkspacePersistenceTests|vp_workspace_persistence_test.cpp|vpRunVpWorkspacePersistenceTest|Widgets"
    "vectorPathEllipseTests|vp_ellipse_test.cpp|vpRunVpEllipseTest|Widgets"
    "vectorPathCurveConstructionTests|vp_curve_construction_test.cpp|vpRunVpCurveConstructionTest|Widgets"
    "vectorPathShortcutManagerTests|vp_shortcut_manager_test.cpp|vpRunVpShortcutManagerTest|Widgets"
    "vectorPathDialogSizePolicyTests|vp_dialog_size_policy_test.cpp|vpRunVpDialogSizePolicyTest|Widgets"
    "vectorPathStandardShapeTests|vp_standard_shape_test.cpp|vpRunVpStandardShapeTest|AppLess"
    "vectorPathLayerColorWorkflowTests|vp_layer_color_workflow_test.cpp|vpRunVpLayerColorWorkflowTest|AppLess"
    "vectorPathPolygonBooleanTests|vp_polygon_boolean_test.cpp|vpRunVpPolygonBooleanTest|AppLess"
    "vectorPathUiScaleTests|vp_ui_scale_test.cpp|vpRunVpUiScaleTest|Widgets"
    "vectorPathToolpathSortTests|vp_toolpath_sort_test.cpp|vpRunVpToolpathSortTest|Gui"
    "vectorPathToolpathSimulationTests|vp_toolpath_simulation_test.cpp|vpRunVpToolpathSimulationTest|Widgets"
    "vectorPathQuickEntityOperationTests|vp_quick_entity_operation_test.cpp|vpRunVpQuickEntityOperationTest|Widgets"
    "vectorPathSvgVectorImportTests|vp_svg_vector_import_test.cpp|vpRunVpSvgVectorImportTest|AppLess"
)
add_executable(vectorPathDesktopTests vp_desktop_test_main.cpp vp_test_runner.h)
target_link_libraries(vectorPathDesktopTests PRIVATE smartGui Qt5::Test)
target_include_directories(vectorPathDesktopTests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set(vp_desktop_registry "")
foreach(vp_suite IN LISTS VP_DESKTOP_SUITES)
    string(REPLACE "|" ";" vp_parts "${vp_suite}")
    list(GET vp_parts 0 vp_name)
    list(GET vp_parts 1 vp_source)
    list(GET vp_parts 2 vp_entry)
    list(GET vp_parts 3 vp_mode)
    target_sources(vectorPathDesktopTests PRIVATE ${vp_source})
    string(APPEND vp_desktop_registry
        "VECTORPATH_DESKTOP_TEST(${vp_name}, ${vp_entry}, ${vp_mode})\n")
    set(vp_platform)
    if(vp_mode STREQUAL "Widgets")
        set(vp_platform "QT_QPA_PLATFORM=offscreen")
    endif()
    add_test(NAME ${vp_name} COMMAND ${CMAKE_COMMAND} -E env
        "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$<TARGET_FILE_DIR:qtadvanceddocking-qt5>;$ENV{PATH}"
        "VECTORPATH_LIBREDWG_DIR=${PROJECT_SOURCE_DIR}/sgraphThirdParty/libredwg-0.14-win64"
        ${vp_platform} "$<TARGET_FILE:vectorPathDesktopTests>" ${vp_name})
    set_tests_properties(${vp_name} PROPERTIES LABELS "desktop")
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/vp_desktop_test_registry.inc" "${vp_desktop_registry}")
