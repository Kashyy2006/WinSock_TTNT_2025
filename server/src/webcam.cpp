#include "webcam.h" // Hoặc #include "../include/webcam.h" tùy cấu trúc folder
#include <windows.h>
#include <vfw.h>
#include <thread>
#include <iostream>

// Lưu ý: Không dùng #pragma comment ở đây vì đã cấu hình trong CMakeLists.txt

// ====================== LOGIC SCREENSHOT ======================

std::string take_screenshot() {
    // 1. Lấy kích thước màn hình
    int x1 = 0;
    int y1 = 0;
    int x2 = GetSystemMetrics(SM_CXSCREEN);
    int y2 = GetSystemMetrics(SM_CYSCREEN);
    int width = x2 - x1;
    int height = y2 - y1;

    // 2. Khởi tạo Device Context
    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDC, hBitmap);

    // 3. Copy màn hình
    BitBlt(hDC, 0, 0, width, height, hScreen, x1, y1, SRCCOPY);

    // 4. Chuẩn bị Header BMP
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
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
    
    // 5. Cấp phát bộ nhớ
    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
    char *lpbitmap = (char *)GlobalLock(hDIB);
    GetDIBits(hScreen, hBitmap, 0, (UINT)bmpScreen.bmHeight, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    // 6. Ghi file
    std::string filename = "screenshot.bmp";
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42; // BM

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    // 7. Dọn dẹp
    CloseHandle(hFile);
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);

    return filename;
}

// ====================== LOGIC WEBCAM ======================

void record_webcam_thread_func(std::string filename, int duration_ms) {
    // Tạo cửa sổ ẩn để capture
    HWND hWndC = capCreateCaptureWindowA("WebcamCapture", WS_POPUP, 0, 0, 320, 240, NULL, 0);
    
    if (hWndC) {
        // Kết nối driver webcam
        if (SendMessage(hWndC, WM_CAP_DRIVER_CONNECT, 0, 0)) {
            
            // Thiết lập file lưu
            SendMessage(hWndC, WM_CAP_FILE_SET_CAPTURE_FILEA, 0, (LPARAM)filename.c_str());

            CAPTUREPARMS CaptureParms;
            SendMessage(hWndC, WM_CAP_GET_SEQUENCE_SETUP, sizeof(CAPTUREPARMS), (LPARAM)&CaptureParms);
            
            // --- CẤU HÌNH FIX LỖI 1 FRAME ---
            
            // 1. Set FPS (Khoảng 15 frames/giây)
            CaptureParms.dwRequestMicroSecPerFrame = (1000000 / 15); 
            
            // 2. Tắt Yield -> Chế độ Blocking (Bắt buộc để quay đủ thời gian trong thread ngầm)
            CaptureParms.fYield = FALSE; 
            
            // 3. Tắt tính năng dừng khi click chuột
            CaptureParms.fAbortLeftMouse = FALSE;
            CaptureParms.fAbortRightMouse = FALSE;
            
            // 4. Các cài đặt khác
            CaptureParms.fMakeUserHitOKToCapture = FALSE;
            CaptureParms.fCaptureAudio = FALSE; // Tắt tiếng để tránh lỗi driver âm thanh
            
            // 5. Giới hạn thời gian quay
            CaptureParms.fLimitEnabled = TRUE;
            CaptureParms.wTimeLimit = duration_ms / 1000;

            // Áp dụng cấu hình
            SendMessage(hWndC, WM_CAP_SET_SEQUENCE_SETUP, sizeof(CAPTUREPARMS), (LPARAM)&CaptureParms);

            // Bắt đầu quay (Sẽ block thread này trong 10s)
            SendMessage(hWndC, WM_CAP_SEQUENCE, 0, 0);

            // Ngắt kết nối
            SendMessage(hWndC, WM_CAP_DRIVER_DISCONNECT, 0, 0);
        }
        DestroyWindow(hWndC);
    }
}

std::string start_webcam_recording() {
    std::string filename = "webcam_video.avi";
    // Chạy thread tách biệt (detach) để server chính không bị đơ
    std::thread t(record_webcam_thread_func, filename, 10000); // 10000ms = 10s
    t.detach();
    
    return filename;
}