#ifndef WEBCAM_H
#define WEBCAM_H

#include <string>

std::string take_screenshot();
void record_webcam_thread_func(std::string filename, int duration_ms);
std::string start_webcam_recording(int duration);

#endif // WEBCAM_H