# Root CMakeLists.txt

cmake_minimum_required(VERSION 3.22)

project(FXML VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
	add_compile_options(/W4 /WX)
else()
	add_compile_options(-Wall -Wextra -Werror)
endif()

add_subdirectory(src)