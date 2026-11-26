cmake_minimum_required(VERSION 3.16)
project(sigmasix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
include(FetchContent)

# Box2D
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG v2.4.1)

# raylib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.0)

FetchContent_MakeAvailable(box2d raylib)

add_executable(game src/main.cpp)
# Permet d'écrire #include <SigmaSix/prefabs/Enemy.hpp> (SigmaSix -> src)
set(SIGMASIX_INC ${CMAKE_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${SIGMASIX_INC})
# En-têtes relais (sans lien symbolique, qui exige des droits sous Windows)
file(GLOB_RECURSE SIGMASIX_HEADERS CONFIGURE_DEPENDS
     RELATIVE ${CMAKE_SOURCE_DIR}/src
     ${CMAKE_SOURCE_DIR}/src/*.hpp ${CMAKE_SOURCE_DIR}/src/*.h)
foreach(header ${SIGMASIX_HEADERS})
  if(NOT EXISTS ${SIGMASIX_INC}/SigmaSix/${header})
    file(WRITE ${SIGMASIX_INC}/SigmaSix/${header}
         "#pragma once\n#include \"${CMAKE_SOURCE_DIR}/src/${header}\"\n")
  endif()
endforeach()
target_include_directories(game PRIVATE ${SIGMASIX_INC})
target_link_libraries(game PRIVATE box2d raylib)

# Assets copiés à côté de l'exécutable (évite les liens symboliques, interdits sans privilège sous Windows)
add_custom_command(TARGET game POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets/shaders $<TARGET_FILE_DIR:game>/assets/shaders)
