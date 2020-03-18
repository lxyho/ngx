cmake_minimum_required(VERSION 3.11.1)

project(NGX)

aux_source_directory(. DIR_SRCS)

add_executable(${project_name} ${DIR_SRCS})