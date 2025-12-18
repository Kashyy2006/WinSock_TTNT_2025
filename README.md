chạy lệnh sau trên terminal
cd server; if (Test-Path build) { Remove-Item build -Recurse -Force }; cmake -B build; cmake --build build; Start-Process ".\build\server.exe" -Verb RunAs

Có gì ko hiểu liên lạc fb Tùng Ngô Thanh