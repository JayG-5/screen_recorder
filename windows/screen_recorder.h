#ifndef SCREEN_RECORDER_H
#define SCREEN_RECORDER_H

#include <vector>
#include <string>

struct MonitorInfo {
    int index;
    std::string name;
    int width;
    int height;
};

#ifdef __cplusplus
extern "C" {
#endif

char* GetMonitorListStr();
void* CreateScreenRecorder(int monitorIndex, const char* outputFilePath);
void StartRecording(void* recorder);
void StopRecording(void* recorder);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_RECORDER_H
