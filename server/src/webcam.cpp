#include <opencv2/opencv.hpp>
#include <mutex>
#include <winsock2.h>
#include <windows.h>
#include <vfw.h> 
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <iostream>

std::string take_screenshot() {
    int x1 = 0; int y1 = 0;
    int x2 = GetSystemMetrics(SM_CXSCREEN);
    int y2 = GetSystemMetrics(SM_CYSCREEN);
    int width = x2 - x1;
    int height = y2 - y1;

    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, width, height, hScreen, x1, y1, SRCCOPY);

    BITMAP bmpScreen;
    GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);
    
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    
    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
    char *lpbitmap = (char *)GlobalLock(hDIB);
    GetDIBits(hScreen, hBitmap, 0, (UINT)bmpScreen.bmHeight, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    std::string filename = "screenshot.bmp"; // Lưu đè vào file này
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42; // BM

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    CloseHandle(hFile);
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);

    return filename;
}

// ====================== LOGIC WEBCAM  ======================
cv::Mat latest_frame;
std::mutex frame_mutex;
std::atomic<bool> webcam_running(false);
std::atomic<bool> is_recording(false); // Cờ kiểm soát việc ghi hình
std::thread webcam_thread;
std::thread record_thread;

// Webcam stream thread
void webcam_stream_thread() {
    // Dùng CAP_DSHOW trên Windows để khởi động nhanh hơn và cho phép chỉnh phân giải tốt hơn
    cv::VideoCapture cap(0, cv::CAP_DSHOW); 
    
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open webcam" << std::endl;
        return;
    }

    // Lưu ý: Nếu camera không hỗ trợ, OpenCV sẽ tự chọn độ phân giải gần nhất
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    cap.set(cv::CAP_PROP_FPS, 15);

    double actual_w = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    double actual_h = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    std::cout << "Camera running at: " << actual_w << "x" << actual_h << std::endl;

    webcam_running = true;
    cv::Mat frame;

    while (webcam_running) {
        cap >> frame;
        if (frame.empty()) continue;

        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            latest_frame = frame.clone(); 
        } 
    }

    cap.release();
    webcam_running = false;
}

void start_webcam() {
    if (!webcam_running) {
        webcam_thread = std::thread(webcam_stream_thread);
        webcam_thread.detach();
    }
}

void stop_webcam() {
    webcam_running = false;
    is_recording = false; 
}

// Get latest frame for MJPEG
bool get_latest_frame(std::vector<uchar>& buffer) {
    cv::Mat temp_frame;
    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        if (latest_frame.empty()) return false;
        temp_frame = latest_frame.clone(); // Copy nhanh để giải phóng mutex sớm
    }

    // TỐI ƯU HÓA: Thiết lập tham số nén JPEG
    std::vector<int> params;
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(90);
    
    // Nếu muốn cực nét nhưng file nặng, dùng 100.
    // Nếu muốn cân bằng tốc độ/chất lượng, dùng 90.

    cv::imencode(".jpg", temp_frame, buffer, params);
    return true;
}

// Record video to AVI
void record_webcam_thread_func(const std::string& filename, int duration_ms) {
    int fps = 15;
    is_recording = true;

    cv::Mat first;
    while (first.empty()) {
        std::lock_guard<std::mutex> lock(frame_mutex);
        if (!latest_frame.empty())
            first = latest_frame.clone();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    cv::VideoWriter writer(
        filename,
        cv::VideoWriter::fourcc('M','J','P','G'),
        fps,
        first.size()
    );
    if (!writer.isOpened()) return;

    int total_frames = fps * duration_ms / 1000;

    for (int i = 0; i < total_frames && is_recording; ++i) {
        cv::Mat frame;
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            if (latest_frame.empty()) continue;
            frame = latest_frame.clone();
        }

        writer.write(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
    }

    writer.release();
    is_recording = false;
}