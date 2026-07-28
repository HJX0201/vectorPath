add_library(smartCommands STATIC
    s_command_catalog.h
    s_command_catalog.cpp
    s_coordinate_input.h
    s_coordinate_input.cpp
    s_i_cad_command.h
)

target_include_directories(smartCommands PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smartCommands PUBLIC smartDocument smartGeometry Qt5::Core)
