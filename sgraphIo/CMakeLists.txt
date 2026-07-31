add_library(smartIo STATIC
    s_file_compatibility_report.h
    s_i_file_codec.h
    s_dxf_codec.h
    s_dxf_codec.cpp
    s_dxf_pair.h
    s_dxf_pair.cpp
    s_dxf_entity_io.h
    s_dxf_entity_io.cpp
    s_dxf_table_io.h
    s_dxf_table_io.cpp
    s_dwg_codec.h
    s_dwg_codec.cpp
    s_dwg_dxf_adapter.h
    s_dwg_dxf_adapter.cpp
    s_dxf_hatch_io.h
    s_dxf_hatch_io.cpp
    s_svg_path_parser.h
    s_svg_path_parser.cpp
    s_svg_vector_data.h
    s_svg_parser.h
    s_svg_parser.cpp
    s_vector_fill.h
    s_vector_fill.cpp
    s_bitmap_vectorizer.h
    s_bitmap_vector_private.h
    s_bitmap_vectorizer.cpp
    s_bitmap_contour_stitcher.cpp
    s_bitmap_run_vectorizer.cpp
    s_vector_document_import.h
    s_vector_document_import.cpp
    s_svg_document_operations.h
    s_svg_document_operations.cpp
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
target_compile_definitions(smartIo PRIVATE
    SMARTCAM_LIBREDWG_DIR="${PROJECT_SOURCE_DIR}/sgraphThirdParty/libredwg-0.14-win64"
)
