add_library(smartDocument STATIC
    s_associative_array_io.h
    s_associative_array_io.cpp
    s_drawing_settings.h
    s_drawing_settings.cpp
    s_dimension_style_record.h
    s_dimension_binary_io.h
    s_dimension_binary_io.cpp
    s_document_style_io.h
    s_document_style_io.cpp
    s_hatch_binary_io.h
    s_hatch_binary_io.cpp
    s_entity_binary_io.h
    s_entity_binary_io.cpp
    s_layer_record.h
    s_plot_style_table.h
    s_plot_style_table.cpp
    s_cad_document.h
    s_cad_document.cpp
    s_cad_document_history.cpp
    s_cad_document_color_layer.cpp
    s_document_audit.h
    s_cad_document_audit.cpp
    s_cad_document_layer_state.cpp
    s_text_style_record.h
    s_cad_document_text_style.cpp
    s_cad_document_dimension_style.cpp
    s_cad_document_io.cpp
    s_document_recovery_manager.h
    s_document_recovery_manager.cpp
    s_document_transaction.h
    s_document_transaction.cpp
    s_toolpath_document.h
    s_toolpath_document.cpp
)

target_include_directories(smartDocument PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartDocument PUBLIC smartCore smartGeometry Qt5::Core Qt5::Gui)
