chạy lệnh sau trên terminal
cd server; if (Test-Path build) { Remove-Item build -Recurse -Force }; cmake -B build; cmake --build build; Start-Process ".\build\server.exe" -Verb RunAs

Có gì ko hiểu liên lạc fb Tùng Ngô Thanh

cd server
Remove-Item build -Recurse -Force -ErrorAction SilentlyContinue
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\server.exe
