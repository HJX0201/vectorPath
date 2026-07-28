add_library(Clipper2 STATIC
    Clipper2Lib/include/clipper2/clipper.core.h
    Clipper2Lib/include/clipper2/clipper.engine.h
    Clipper2Lib/include/clipper2/clipper.export.h
    Clipper2Lib/include/clipper2/clipper.h
    Clipper2Lib/include/clipper2/clipper.minkowski.h
    Clipper2Lib/include/clipper2/clipper.offset.h
    Clipper2Lib/include/clipper2/clipper.rectclip.h
    Clipper2Lib/include/clipper2/clipper.triangulation.h
    Clipper2Lib/include/clipper2/clipper.version.h
    Clipper2Lib/src/clipper.engine.cpp
    Clipper2Lib/src/clipper.offset.cpp
    Clipper2Lib/src/clipper.rectclip.cpp
    Clipper2Lib/src/clipper.triangulation.cpp
)

target_compile_features(Clipper2 PUBLIC cxx_std_17)
target_compile_definitions(Clipper2 PUBLIC CLIPPER2_MAX_DECIMAL_PRECISION=8)
target_include_directories(Clipper2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Clipper2Lib/include
)

if(MSVC)
    target_compile_options(Clipper2 PRIVATE /W3)
endif()

set_target_properties(Clipper2 PROPERTIES FOLDER "smartThirdParty")
