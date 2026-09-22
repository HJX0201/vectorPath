add_library(smartGeometryCore STATIC
    vp_drafting_state.h
    vp_drafting_state.cpp
    vp_geometry_types.h
    vp_geometry_types.cpp
    vp_curve_entities.h
    vp_standard_shape.h
    vp_standard_shape.cpp
    vp_polygon_boolean.h
    vp_polygon_boolean.cpp
    vp_polygon_geometry.h
    vp_polygon_geometry.cpp
    vp_spline_geometry.h
    vp_spline_geometry.cpp
    vp_ellipse_geometry.h
    vp_ellipse_geometry.cpp
)

target_include_directories(smartGeometryCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartGeometryCore PUBLIC smartCore PRIVATE Clipper2)
set_target_properties(smartGeometryCore PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

if(VECTORPATH_BUILD_DESKTOP)
    add_library(smartGeometry STATIC
        vp_qt_geometry.h
        vp_qt_geometry.cpp
        vp_qt_text.h
        vp_entity.h
        vp_dimension_geometry.h
        vp_dimension_geometry.cpp
        vp_hatch_geometry.h
        vp_hatch_geometry.cpp
        vp_toolpath.h
        vp_toolpath.cpp
    )

    target_include_directories(smartGeometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(smartGeometry PUBLIC smartGeometryCore Qt5::Core Qt5::Gui)
endif()
