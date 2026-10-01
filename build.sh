#!/bin/bash
echo "Building NexusProxy..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
echo
echo "Build complete. Run with:"
echo "  ./build/NexusProxy [config.json]"
