# /src CMakeLists.txt

# GTest
# https://github.com/google/googletest/tree/main/googletest#incorporating-into-an-existing-cmake-project

include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/504ea69cf7e9947be54f808a09b7b08988e84b5f.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(XFML_LIB_STATIC STATIC FXML.cpp)

add_executable(FXML_TESTS tests.cpp)
target_link_libraries(FXML_TESTS PRIVATE XFML_LIB_STATIC gtest_main)
add_test(NAME FXML_TESTS COMMAND example)