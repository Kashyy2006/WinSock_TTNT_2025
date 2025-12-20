cmake_minimum_required(VERSION 3.15)
project(OpenCV_Project)

# -- 1. Cấu hình C++ Standard --
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -- 2. Cấu hình OpenCV --
set(OpenCV_DIR "C:/opencv/build")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# -- 3. Tạo Target trước (Quan trọng: Phải tạo target rồi mới chỉnh đường dẫn cho nó) --
add_executable(server all_in_one.cpp)

# -- 4. Ép đường dẫn xuất file (FIX LỖI CỦA BẠN TẠI ĐÂY) --

set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}"
)

# -- 5. Link thư viện --
target_link_libraries(server
    ${OpenCV_LIBS}
    ws2_32
    gdi32
    vfw32
)