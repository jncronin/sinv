cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(sinv C CXX)

find_package(SDL2 CONFIG REQUIRED)

add_executable(sinv)

target_sources(sinv
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
)

target_include_directories(sinv
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        "${SDL2_INCLUDE_DIRS}"
)

target_link_libraries(sinv
    PRIVATE
        SDL2::SDL2main
        SDL2::SDL2
)

if(GAMEKID)
    gk_generate_package(sinv)
endif()
