cmake_minimum_required(VERSION 3.0)

project(jsoncpp)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES_JSONCPP
        "src/lib_json/json_batchallocator.h"
        "src/lib_json/json_internalarray.inl"
        "src/lib_json/json_internalmap.inl"
        "src/lib_json/json_reader.cpp"
        "src/lib_json/json_value.cpp"
        "src/lib_json/json_valueiterator.inl"
        "src/lib_json/json_writer.cpp"
)

add_library(jsoncpp ${SOURCES_JSONCPP})

target_include_directories(jsoncpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)
