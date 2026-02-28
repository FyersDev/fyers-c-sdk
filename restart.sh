#!/bin/bash

cd /path/to/fyers-c-sdk/build || exit 1

# Run cmake to ensure build files are up to date (BUILD_EXAMPLES=ON for symbol_update)
cmake .. -DBUILD_EXAMPLES=ON || exit 1

# Make sample_code
echo "Building sample_code..."
make sample_code || exit 1

echo "Running sample_code..."
./examples/sample_code 

# Make on_position
# echo "Building on_position..."
# make on_position || exit 1

# echo "Running on_position..."
# ./examples/on_position