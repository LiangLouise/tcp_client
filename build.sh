#!/bin/bash

# Set the name of the executable
EXECUTABLE="tcp_echo_client"

# Set the source file
SOURCE_FILE="tcp_echo_client.c"

# Set the flags for the libuv library
LIBUV_FLAGS="-luv"

# Compile the program
gcc -g -o $EXECUTABLE $SOURCE_FILE $LIBUV_FLAGS

# Check if the compilation was successful
if [ $? -eq 0 ]; then
    echo "Build successful. Executable: ./$EXECUTABLE"
else
    echo "Build failed."
fi
