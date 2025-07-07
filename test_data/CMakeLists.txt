# /test_data CMakeLists.txt

message("Copying files")
file(GLOB test_files ${CMAKE_CURRENT_SOURCE_DIR}/*.xml)
foreach(in_file IN LISTS test_files)
    get_filename_component(out_file ${in_file} NAME)
    message("Copying ${out_file} to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_data")
    file(COPY_FILE ${in_file} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_data/${out_file} RESULT result)
    message("Result ${result}")
endforeach()
