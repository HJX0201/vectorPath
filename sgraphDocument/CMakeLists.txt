add_library(smartDocument STATIC
    vp_associative_array_io.h
    vp_associative_array_io.cpp
    vp_drawing_settings.h
    vp_drawing_settings.cpp
    vp_dimension_style_record.h
    vp_dimension_binary_io.h
    vp_dimension_binary_io.cpp
    vp_document_style_io.h
    vp_document_style_io.cpp
    vp_hatch_binary_io.h
    vp_hatch_binary_io.cpp
    vp_entity_binary_io.h
    vp_entity_binary_io.cpp
    vp_layer_record.h
    vp_plot_style_table.h
    vp_plot_style_table.cpp
    vp_cad_document.h
    vp_cad_document.cpp
    vp_cad_document_history.cpp
    vp_cad_document_color_layer.cpp
    vp_document_audit.h
    vp_cad_document_audit.cpp
    vp_cad_document_layer_state.cpp
    vp_text_style_record.h
    vp_cad_document_text_style.cpp
    vp_cad_document_dimension_style.cpp
    vp_cad_document_io.cpp
    vp_document_recovery_manager.h
    vp_document_recovery_manager.cpp
    vp_document_transaction.h
    vp_document_transaction.cpp
    vp_toolpath_document.h
    vp_toolpath_document.cpp
)

target_include_directories(smartDocument PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartDocument PUBLIC smartCore smartGeometry Qt5::Core Qt5::Gui)
