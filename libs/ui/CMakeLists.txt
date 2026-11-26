include(FetchContent)
FetchContent_Declare(raygui
  GIT_REPOSITORY https://github.com/raysan5/raygui.git
  GIT_TAG 5.0)
FetchContent_MakeAvailable(raygui)

add_library(SigmaUi STATIC src/Ui.cpp)
target_include_directories(SigmaUi PUBLIC include ${raygui_SOURCE_DIR}/src)
target_link_libraries(SigmaUi PUBLIC SigmaYaml raylib)
