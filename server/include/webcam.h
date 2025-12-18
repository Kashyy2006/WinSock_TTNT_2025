#ifndef WEBCAM_H
#define WEBCAM_H

#include <string>

/**
 * Chụp ảnh màn hình hiện tại.
 * Lưu thành file "screenshot.bmp".
 * @return std::string Tên file đã lưu.
 */
std::string take_screenshot();

/**
 * Bắt đầu quay video từ webcam mặc định trong 10 giây.
 * Chạy trên một luồng (thread) riêng để không chặn server.
 * Lưu thành file "webcam_video.avi".
 * @return std::string Tên file video.
 */
std::string start_webcam_recording();

#endif // WEBCAM_H