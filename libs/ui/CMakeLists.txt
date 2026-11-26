include(FetchContent)
FetchContent_Declare(raygui
  GIT_REPOSITORY https://github.com/raysan5/raygui.git
  GIT_TAG 5.0)
FetchContent_MakeAvailable(raygui)

# Copie de raygui.h avec un hook sur GuiDrawRectangle (bords arrondis), sans toucher aux sources téléchargées
set(RAYGUI_PATCHED ${CMAKE_CURRENT_BINARY_DIR}/raygui/raygui.h)
file(READ ${raygui_SOURCE_DIR}/src/raygui.h RAYGUI_SRC)
string(REGEX REPLACE
  "(static void GuiDrawRectangle\\(Rectangle rec, int borderWidth, Color borderColor, Color color\\)\r?\n{\r?\n)"
  "\\1#if defined(RAYGUI_DRAW_RECTANGLE_HOOK)\n    if (RAYGUI_DRAW_RECTANGLE_HOOK(rec, borderWidth, GuiFade(borderColor, guiAlpha), GuiFade(color, guiAlpha))) return;\n#endif\n"
  RAYGUI_SRC "${RAYGUI_SRC}")
if(NOT RAYGUI_SRC MATCHES "RAYGUI_DRAW_RECTANGLE_HOOK")
  message(FATAL_ERROR "SigmaUi : impossible d'injecter le hook dans raygui.h")
endif()
if(EXISTS ${RAYGUI_PATCHED})
  file(READ ${RAYGUI_PATCHED} RAYGUI_OLD)
endif()
if(NOT "${RAYGUI_OLD}" STREQUAL "${RAYGUI_SRC}")
  file(WRITE ${RAYGUI_PATCHED} "${RAYGUI_SRC}")
endif()

add_library(SigmaUi STATIC src/Ui.cpp)
target_include_directories(SigmaUi PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/raygui)
target_link_libraries(SigmaUi PUBLIC SigmaYaml raylib)
