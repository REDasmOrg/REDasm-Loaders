cmake_minimum_required(VERSION 3.25)
project(ESP)

find_package(REDasm REQUIRED)

file(GLOB_RECURSE HEADERS *.h*)
file(GLOB_RECURSE SOURCES *.cpp)

redasm_plugin(${PROJECT_NAME} ${LOADER_TYPE} ${HEADERS} ${SOURCES})
