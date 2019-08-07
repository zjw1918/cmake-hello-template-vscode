cmake_minimum_required(VERSION 3.0.0)
project(Hello VERSION 0.1.0)

include(CTest)
enable_testing()

aux_source_directory(. src_dir)
aux_source_directory(func func_dir)

add_subdirectory(parse)

add_executable(Hello ${src_dir} ${func_dir})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(Hello parse)

