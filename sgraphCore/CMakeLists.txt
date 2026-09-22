add_library(smartCore INTERFACE)
target_sources(smartCore INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vp_result.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vp_id_collection.h)
target_include_directories(smartCore INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(smartCore INTERFACE cxx_std_17)
