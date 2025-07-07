# /test_data CMakeLists.txt

message("Copying files")
file(GLOB test_files ${CMAKE_CURRENT_SOURCE_DIR}/*.*)
foreach(in_file IN LISTS test_files)
    get_filename_component(out_file ${in_file} NAME)
    file(COPY_FILE ${in_file} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${out_file})
endforeach()
