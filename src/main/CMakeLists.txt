# Main executable
add_executable(yoki_chess_engine
    main.cpp
)

# Link to parent include directories
target_include_directories(yoki_chess_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link with engine and board libraries
target_link_libraries(yoki_chess_engine engine board)