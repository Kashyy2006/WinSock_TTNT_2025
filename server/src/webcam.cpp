#include <winsock2.h>
#include <windows.h>
#include <vfw.h> 
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <iostream>

// ====================== LOGIC SCREENSHOT ======================
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

// ====================== LOGIC WEBCAM ======================
void record_webcam_thread_func(std::string filename, int duration_ms) {
    std::cout << "[Webcam] Dang khoi tao cua so..." << std::endl;

    HWND hWndC = capCreateCaptureWindowA(
        "Webcam Recorder", 
        WS_VISIBLE | WS_OVERLAPPEDWINDOW, 
        100, 100, 320, 240, 
        NULL, 0
    );

    if (!hWndC) {
        std::cerr << "[ERROR] Khong the tao cua so Webcam!" << std::endl;
        return;
    }

    bool isConnected = false;

    // --- VÒNG LẶP QUÉT DRIVER (Từ 0 đến 9) ---
    for (int driverIndex = 0; driverIndex < 10; driverIndex++) {
        if (SendMessage(hWndC, WM_CAP_DRIVER_CONNECT, driverIndex, 0)) {
            std::cout << "[Webcam] Ket noi THANH CONG voi Driver index: " << driverIndex << std::endl;
            isConnected = true;
            break; // Tìm thấy rồi thì thoát vòng lặp
        }
    }

    if (isConnected) {
        // Cấu hình Preview
        SendMessage(hWndC, WM_CAP_SET_SCALE, TRUE, 0);
        SendMessage(hWndC, WM_CAP_SET_PREVIEWRATE, 66, 0);
        SendMessage(hWndC, WM_CAP_SET_PREVIEW, TRUE, 0);

        // Thiết lập file lưu (Dùng đường dẫn tuyệt đối nếu cần)
        if (SendMessage(hWndC, WM_CAP_FILE_SET_CAPTURE_FILEA, 0, (LPARAM)filename.c_str())) {
             std::cout << "[Webcam] File save location: " << filename << std::endl;
        }

        CAPTUREPARMS CaptureParms;
        SendMessage(hWndC, WM_CAP_GET_SEQUENCE_SETUP, sizeof(CAPTUREPARMS), (LPARAM)&CaptureParms);

        CaptureParms.dwRequestMicroSecPerFrame = (1000000 / 15); // 15 FPS
        CaptureParms.fYield = TRUE; 
        CaptureParms.fAbortLeftMouse = FALSE;
        CaptureParms.fAbortRightMouse = FALSE;
        CaptureParms.fMakeUserHitOKToCapture = FALSE;
        CaptureParms.fCaptureAudio = FALSE; 
        CaptureParms.fLimitEnabled = TRUE;
        CaptureParms.wTimeLimit = duration_ms / 1000;

        SendMessage(hWndC, WM_CAP_SET_SEQUENCE_SETUP, sizeof(CAPTUREPARMS), (LPARAM)&CaptureParms);

        std::cout << "[Webcam] Dang quay (" << duration_ms/1000 << "s)..." << std::endl;
        
        if (SendMessage(hWndC, WM_CAP_SEQUENCE, 0, 0)) {
            std::cout << "[Webcam] Xong! File da duoc tao." << std::endl;
        } else {
            std::cerr << "[ERROR] Loi trong qua trinh ghi file." << std::endl;
        }

        SendMessage(hWndC, WM_CAP_DRIVER_DISCONNECT, 0, 0);
    } 
    else {
        // Nếu quét từ 0-9 mà vẫn không được
        std::cerr << "[CRITICAL ERROR] Khong tim thay bat ky Webcam nao (Index 0-9)!" << std::endl;
        std::cerr << "Loi khuyen: Kiem tra quyen Privacy hoac Driver." << std::endl;
    }

    DestroyWindow(hWndC);
}

std::string start_webcam_recording(int duration_sec) {
    std::string filename = "evidence_" + std::to_string(duration_sec) + "s.avi";

    record_webcam_thread_func(filename, duration_sec * 1000);

    return "/" + filename;
}
