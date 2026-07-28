add_library(smartGeometry STATIC
    s_geometry_types.h
    s_geometry_types.cpp
    s_standard_shape.h
    s_standard_shape.cpp
    s_polygon_boolean.h
    s_polygon_boolean.cpp
    s_entity.h
    s_dimension_geometry.h
    s_dimension_geometry.cpp
    s_hatch_geometry.h
    s_hatch_geometry.cpp
    s_spline_geometry.h
    s_spline_geometry.cpp
    s_ellipse_geometry.h
    s_ellipse_geometry.cpp
    s_toolpath.h
    s_toolpath.cpp
)

target_include_directories(smartGeometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartGeometry PUBLIC smartCore Clipper2 Qt5::Core Qt5::Gui)
