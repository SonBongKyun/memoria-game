cmake_minimum_required(VERSION 3.24)
project(MemoriaMemoryParity LANGUAGES CXX)
enable_testing()
add_executable(MemoriaMemoryParity
    MemoryParityMain.cpp
    ../../Memoria/Source/Memoria/Private/Domain/MemoriaMemoryModel.cpp)
target_compile_features(MemoriaMemoryParity PRIVATE cxx_std_20)
target_compile_definitions(MemoriaMemoryParity PRIVATE MEMORIA_API=)
target_include_directories(MemoriaMemoryParity PRIVATE
    ../../Memoria/Source/Memoria/Public ../Shared ../Generated)
if(MSVC)
    target_compile_options(MemoriaMemoryParity PRIVATE /W4 /WX /permissive- /utf-8 /bigobj)
else()
    target_compile_options(MemoriaMemoryParity PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME Memoria.MemoryParity COMMAND MemoriaMemoryParity)
