# Engine Binary CMakeLists.txt

# Define the engine executable
add_executable(yoki-engine
    main.cpp
)

# Link with the core library
target_link_libraries(yoki-engine
    PRIVATE yoki-core
)

# Set target properties
set_target_properties(yoki-engine PROPERTIES
    OUTPUT_NAME "yoki-engine"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install target
install(TARGETS yoki-engine
    RUNTIME DESTINATION bin
)