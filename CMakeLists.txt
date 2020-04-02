cmake_minimum_required (VERSION 3.15.1)
project(Tinker)

set(SOURCE_LIST
    "Source/main.cpp"
)

add_executable(Tinker ${SOURCE_LIST})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST})
install (TARGETS Tinker DESTINATION bin)
