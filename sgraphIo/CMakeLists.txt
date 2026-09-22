add_library(smartIo STATIC
    vp_file_compatibility_report.h
    vp_i_file_codec.h
    vp_dxf_codec.h
    vp_dxf_codec.cpp
    vp_dxf_pair.h
    vp_dxf_pair.cpp
    vp_dxf_entity_io.h
    vp_dxf_entity_io.cpp
    vp_dxf_table_io.h
    vp_dxf_table_io.cpp
    vp_dwg_codec.h
    vp_dwg_codec.cpp
    vp_dwg_dxf_adapter.h
    vp_dwg_dxf_adapter.cpp
    vp_dxf_hatch_io.h
    vp_dxf_hatch_io.cpp
    vp_svg_path_parser.h
    vp_svg_path_parser.cpp
    vp_svg_vector_data.h
    vp_svg_parser.h
    vp_svg_parser.cpp
    vp_vector_fill.h
    vp_vector_fill.cpp
    vp_bitmap_vectorizer.h
    vp_bitmap_vector_private.h
    vp_bitmap_vectorizer.cpp
    vp_bitmap_contour_stitcher.cpp
    vp_bitmap_run_vectorizer.cpp
    vp_vector_document_import.h
    vp_vector_document_import.cpp
    vp_svg_document_operations.h
    vp_svg_document_operations.cpp
)

target_include_directories(smartIo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartIo PUBLIC
    smartCore
    smartGeometry
    smartDocument
    Qt5::Core
    Qt5::Gui
    Qt5::Concurrent
)
target_link_libraries(smartIo PRIVATE Clipper2)
