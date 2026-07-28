add_executable(smartBitmapVectorBenchmark
    src/s_bitmap_benchmark_types.h
    src/s_bitmap_benchmark_generator.h
    src/s_bitmap_benchmark_generator.cpp
    src/s_bitmap_flood_baseline.h
    src/s_bitmap_flood_baseline.cpp
    src/s_bitmap_benchmark_validation.h
    src/s_bitmap_benchmark_validation.cpp
    src/s_bitmap_benchmark_report.h
    src/s_bitmap_benchmark_report.cpp
    src/s_bitmap_vector_benchmark.cpp
)

target_link_libraries(smartBitmapVectorBenchmark
    PRIVATE
        smartIo
        Qt5::Core
        Qt5::Gui
        Qt5::Concurrent
)

target_compile_definitions(smartBitmapVectorBenchmark PRIVATE
    SGRAPH_BENCHMARK_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    SGRAPH_BENCHMARK_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(WIN32)
    target_link_libraries(smartBitmapVectorBenchmark PRIVATE Psapi)
endif()

add_test(
    NAME smartCadBitmapVectorBenchmarkSmoke
    COMMAND ${CMAKE_COMMAND} -E env
            "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$ENV{PATH}"
            "$<TARGET_FILE:smartBitmapVectorBenchmark>"
            --cases 20
            --seed 20260727
            --threads 2
            --repetitions 1
            --smoke
            --output "${CMAKE_CURRENT_BINARY_DIR}/bitmap_benchmark_smoke"
)

set_tests_properties(
    smartCadBitmapVectorBenchmarkSmoke
    PROPERTIES LABELS "bitmap_vector_benchmark"
)
