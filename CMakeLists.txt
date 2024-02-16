cmake_minimum_required(VERSION 3.10)

project(JermpEssentials)
set(PROJECT_VENDOR "Giulio Ermanno Pibiri")
set(PROJECT_URL "https://github.com/jermp/essentials")
set(PROJECT_DESCRIPTION "Essential C++ utilities ")

if(NOT TARGET JermpEssentials)
    add_library(JermpEssentials INTERFACE)
    target_include_directories(JermpEssentials INTERFACE include)
endif()
