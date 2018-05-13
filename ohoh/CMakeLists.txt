cmake_minimum_required(VERSION 3.10)
project(operating_system_hw2 C)

set(CMAKE_C_STANDARD 99)

add_executable(operating_system_hw2 main.c fs.c fs.h mount.c disk.h disk.c)