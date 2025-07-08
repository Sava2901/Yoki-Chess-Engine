# Core library for shared chess logic
add_library(yoki-core STATIC
    board.cpp
    board.h
    movegen.cpp
    movegen.h
    search.cpp
    search.h
    uci.cpp
    uci.h
    utils.cpp
    utils.h
)

target_include_directories(yoki-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})