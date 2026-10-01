@echo off
echo Building NexusProxy...
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
echo.
echo Build complete. Run with:
echo   build\Release\NexusProxy.exe [config.json]
