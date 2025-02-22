cmake_minimum_required(VERSION 3.0)

project(jsoncpp)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

set(SOURCES_JSONCPP
        "src/lib_json/json_batchallocator.h"
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



set(SOURCES_JSONTESTRUNNER
        "src/jsontestrunner/main.cpp"
)

add_executable(jsontestrunner ${SOURCES_JSONTESTRUNNER})

target_link_libraries(jsontestrunner PRIVATE jsoncpp)

target_include_directories(jsontestrunner PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)