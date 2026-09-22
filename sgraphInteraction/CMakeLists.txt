add_library(smartInteraction STATIC
    vp_drafting_state.h
    vp_drafting_state.cpp
)

target_include_directories(smartInteraction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartInteraction PUBLIC smartGeometryCore)
target_compile_features(smartInteraction PUBLIC cxx_std_17)

set_target_properties(smartInteraction PROPERTIES AUTOMOC OFF AUTORCC OFF AUTOUIC OFF)
