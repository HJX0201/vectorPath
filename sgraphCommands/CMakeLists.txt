add_library(smartCommands STATIC
    vp_command_catalog.h
    vp_command_catalog.cpp
    vp_coordinate_input.h
    vp_coordinate_input.cpp
    vp_i_cad_command.h
)

target_include_directories(smartCommands PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartCommands PUBLIC smartDocument smartGeometry Qt5::Core)
