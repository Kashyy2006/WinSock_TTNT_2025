#ifndef WEBCAM_H
#define WEBCAM_H

#include <vector>
#include <string>


std::string take_screenshot();
void webcam_stream_thread();
void start_webcam();
void stop_webcam();
bool get_latest_frame(std::vector<uchar>& buffer);
void record_webcam_thread_func(const std::string& filename, int duration_ms);

#endif // WEBCAM_H