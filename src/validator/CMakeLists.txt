# Validator Binary CMakeLists.txt

# Define the validator executable
add_executable(yoki-validator
    main.cpp
)

# Link with the core library
target_link_libraries(yoki-validator
    PRIVATE yoki-core
)

# Set target properties
set_target_properties(yoki-validator PROPERTIES
    OUTPUT_NAME "yoki-validator"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install target
install(TARGETS yoki-validator
    RUNTIME DESTINATION bin
)