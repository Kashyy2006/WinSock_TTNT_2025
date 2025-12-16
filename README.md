Có 2 cách chạy:

Cách 1: chạy trực tiếp trên all_in_one.cpp (đây là file tổng hợp tất cả các code của server)
dùng lệnh "g++ all_in_one.cpp -o all_in_one.exe -lws2_32" trong terminal
Rồi dùng "./all_in_one.exe" để chạy chương trình trên localhost<PORT>

Cách 2:
cd đến thư mục server bằng "cd server" (trên terminal)
rồi chạy lần lượt 2 lệnh trên terminal
    cmake -S . -B build
    cmake --build build
dùng "./build/server.exe" để chạy.

Có gì ko hiểu liên lạc fb Tùng Ngô Thanh