include_directories(SPP3)

aux_source_directory(. src_lib)

add_library(parse_spo_pr ${src_lib})