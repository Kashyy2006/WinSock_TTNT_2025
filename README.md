Phần A: Cài đặt openCV

- Vào link "https://opencv.org/releases/" và tải openCV 4.12.0 cho windows
- Chạy file "opencv-4.12.0-windows" trong repo để cài opencv
- Cài vào ổ đĩa C:/ sao cho thư mục openCV có đường dẫn: "C:\opencv" (nếu không phải sửa thêm file CMakeLists.txt để chạy)
- Vào thanh Search tìm kiếm "Environments Variables"
- Chọn Path rồi Edit
- Chọn New rồi thêm đưỡng dẫn "C:\opencv\build\x64\vc16\bin"
- reset IDE để load openCV

Phần B: Chạy code

- (Cài CMake nếu chưa có)
- Copy đoạn code dưới vào terminal (terminal chạy powershell)

cd server
Remove-Item build -Recurse -Force -ErrorAction SilentlyContinue
Stop-Process -Name "server" -Force -ErrorAction SilentlyContinue; if (Test-Path .\server.exe) { Remove-Item .\server.exe -Force }
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\server.exe

- Tiếp theo, ở máy client bật file index.html trong thư mục frontend, kết nối với IP của server và sử dụng các chức năng
  ( Chỉ dùng bên trong mạng Lan do việc mở PORT thực tế trên router khá nguy hiểm, nếu cần có thể dùng VPN để tạo mạng LAN ảo, ví dụ như Radmin VPN. )
