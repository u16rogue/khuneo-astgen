cmake_minimum_required(VERSION 3.18)

if (NOT TARGET kh_core)
  message(FATAL_ERROR "-- khuneo > astgen > kh_astgen requires kh_core as an available target. Check your build to load khuneo-core first.")
endif()

project(kh_astgen)

add_library(
  ${PROJECT_NAME}
  STATIC
  "include/kh-astgen/lexer.h"
  "src/lexer.c"
  "include/kh-astgen/parser.h"
  "src/parser.c"
  "include/kh-astgen/ast.h"
  "src/ast.c"
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  C_STANDARD 17
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  "include/"
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  kh_core
)
