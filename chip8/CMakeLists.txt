cmake_minimum_required(VERSION 3.12)
project(CHIP8)

find_package(REDasm REQUIRED)

file(GLOB_RECURSE HEADERS *.h*)
file(GLOB_RECURSE SOURCES *.cpp)

redasm_plugin(${PROJECT_NAME} ${LOADER_TYPE} ${REDASM_LOADER} ${HEADERS} ${SOURCES})
