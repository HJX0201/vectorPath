if(NOT DEFINED OUTPUT_DIRECTORY)
    message(FATAL_ERROR "OUTPUT_DIRECTORY is required.")
endif()

if(NOT EXISTS "${OUTPUT_DIRECTORY}/manifest.json")
    message(FATAL_ERROR "Benchmark manifest is missing.")
endif()

if(NOT EXISTS "${OUTPUT_DIRECTORY}/bitmap_vector_benchmark_report.html")
    message(FATAL_ERROR "Benchmark HTML report is missing.")
endif()

if(EXISTS "${OUTPUT_DIRECTORY}/cases")
    message(FATAL_ERROR "Generated PNG cases were not cleaned.")
endif()

if(EXISTS "${OUTPUT_DIRECTORY}/failures")
    message(FATAL_ERROR "Failure diagnostics were not cleaned.")
endif()

file(GLOB output_entries
    RELATIVE "${OUTPUT_DIRECTORY}"
    "${OUTPUT_DIRECTORY}/*"
)
list(SORT output_entries)
set(expected_entries
    bitmap_vector_benchmark_report.html
    manifest.json
)
list(SORT expected_entries)

if(NOT output_entries STREQUAL expected_entries)
    message(FATAL_ERROR
        "Successful benchmark output must contain only the manifest and report. "
        "Actual entries: ${output_entries}"
    )
endif()
