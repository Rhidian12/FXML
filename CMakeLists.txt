# Root CMakeLists.txt

cmake_minimum_required(VERSION 3.22)

project(FXML VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

if(MSVC)
	add_compile_options(/W4) # /Wx
else()
	add_compile_options(-Wall -Wextra) # -Werror
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
add_subdirectory(test_data)