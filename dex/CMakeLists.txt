cmake_minimum_required(VERSION 3.10)
project(DEX)

find_package(REDasm REQUIRED)

file(GLOB_RECURSE HEADERS assembler/*.h loader/*.h demangler.h common_types.h)
file(GLOB_RECURSE SOURCES assembler/*.cpp loader/*.cpp demangler.cpp plugin.cpp)

add_subdirectory(android)
redasm_plugin(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE LibDEX)
