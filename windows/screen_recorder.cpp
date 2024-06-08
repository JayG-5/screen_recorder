#include "screen_recorder.h"
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <shlwapi.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <ctime>

using namespace Microsoft::WRL;

// Monitor enumeration callback function
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
std::vector<MonitorInfo>* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);
MONITORINFOEXA monitorInfo;
monitorInfo.cbSize = sizeof(MONITORINFOEXA);
if (GetMonitorInfoA(hMonitor, &monitorInfo)) {
MonitorInfo info;
info.index = static_cast<int>(monitors->size());
info.name = monitorInfo.szDevice;
info.width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
info.height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
monitors->push_back(info);
}
return TRUE;
}

extern "C" char* GetMonitorListStr() {
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
    std::string result;
    for (const auto& monitor : monitors) {
        result += std::to_string(monitor.index) + "," + monitor.name + "," +
                  std::to_string(monitor.width) + "," + std::to_string(monitor.height) + ";";
    }

    char* cstr = new char[result.length() + 1];
    std::strcpy(cstr, result.c_str());
    return cstr;
}

// Screen Recorder class
class ScreenRecorder {
public:
    ScreenRecorder(int monitorIndex, const std::string& outputFilePath) :
            monitorIndex_(monitorIndex), outputFilePath_(outputFilePath) {
        CoInitialize(nullptr);
        MFStartup(MF_VERSION);
    }

    ~ScreenRecorder() {
        MFShutdown();
        CoUninitialize();
    }

    void StartRecording() {
        recording_ = true;
        recordingThread_ = std::thread(&ScreenRecorder::Record, this);
    }

    void StopRecording() {
        recording_ = false;
        if (recordingThread_.joinable()) {
            recordingThread_.join();
        }
    }

private:
    void Record() {
        // Initialize Direct3D
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        ComPtr<ID3D11Device> d3dDevice;
        ComPtr<ID3D11DeviceContext> d3dContext;
        D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                          &featureLevel, 1, D3D11_SDK_VERSION, &d3dDevice, nullptr, &d3dContext);

        // Get DXGI Output
        ComPtr<IDXGIFactory1> dxgiFactory;
        CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory.GetAddressOf()));

        ComPtr<IDXGIAdapter1> dxgiAdapter;
        dxgiFactory->EnumAdapters1(0, dxgiAdapter.GetAddressOf());

        ComPtr<IDXGIOutput> dxgiOutput;
        dxgiAdapter->EnumOutputs(monitorIndex_, dxgiOutput.GetAddressOf());

        ComPtr<IDXGIOutput1> dxgiOutput1;
        dxgiOutput.As(&dxgiOutput1);

        ComPtr<IDXGIOutputDuplication> dxgiOutputDupl;
        dxgiOutput1->DuplicateOutput(d3dDevice.Get(), dxgiOutputDupl.GetAddressOf());

        // Initialize Media Foundation
        IMFAttributes* attributes;
        MFCreateAttributes(&attributes, 1);
        attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

        ComPtr<IMFSinkWriter> sinkWriter;
        MFCreateSinkWriterFromURL(outputFilePath_.c_str(), nullptr, attributes, &sinkWriter);

        // Configure the output media type
        ComPtr<IMFMediaType> outputMediaType;
        MFCreateMediaType(&outputMediaType);
        outputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        outputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
        outputMediaType->SetUINT32(MF_MT_AVG_BITRATE, 800000);
        outputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        MFSetAttributeSize(outputMediaType.Get(), MF_MT_FRAME_SIZE, 1920, 1080);
        MFSetAttributeRatio(outputMediaType.Get(), MF_MT_FRAME_RATE, 30, 1);
        MFSetAttributeRatio(outputMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

        sinkWriter->AddStream(outputMediaType.Get(), &streamIndex_);

        // Configure the input media type
        ComPtr<IMFMediaType> inputMediaType;
        MFCreateMediaType(&inputMediaType);
        inputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        inputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        inputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        MFSetAttributeSize(inputMediaType.Get(), MF_MT_FRAME_SIZE, 1920, 1080);
        MFSetAttributeRatio(inputMediaType.Get(), MF_MT_FRAME_RATE, 30, 1);
        MFSetAttributeRatio(inputMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

        sinkWriter->SetInputMediaType(streamIndex_, inputMediaType.Get(), nullptr);

        sinkWriter->BeginWriting();

        // Capture loop
        while (recording_) {
            DXGI_OUTDUPL_FRAME_INFO frameInfo;
            ComPtr<IDXGIResource> desktopResource;
            dxgiOutputDupl->AcquireNextFrame(1000 / 30, &frameInfo, desktopResource.GetAddressOf());

            ComPtr<ID3D11Texture2D> texture;
            desktopResource.As(&texture);

            // Map texture to memory
            D3D11_TEXTURE2D_DESC textureDesc;
            texture->GetDesc(&textureDesc);

            D3D11_MAPPED_SUBRESOURCE mappedResource;
            d3dContext->Map(texture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);

            // Copy texture data to frame
            IMFMediaBuffer* buffer;
            MFCreateMemoryBuffer(textureDesc.Width * textureDesc.Height * 4, &buffer);

            BYTE* dstData;
            DWORD maxLength, currentLength;
            buffer->Lock(&dstData, &maxLength, &currentLength);
            std::memcpy(dstData, mappedResource.pData, textureDesc.Width * textureDesc.Height * 4);
            buffer->Unlock();
            buffer->SetCurrentLength(textureDesc.Width * textureDesc.Height * 4);

            IMFSample* sample;
            MFCreateSample(&sample);
            sample->AddBuffer(buffer);
            sample->SetSampleTime(frameInfo.LastPresentTime.QuadPart);
            sample->SetSampleDuration(frameInfo.AccumulatedFrames * 10000000 / 30);

            sinkWriter->WriteSample(streamIndex_, sample);

            sample->Release();
            buffer->Release();

            d3dContext->Unmap(texture.Get(), 0);
            dxgiOutputDupl->ReleaseFrame();
        }

        sinkWriter->Finalize();
    }

    int monitorIndex_;
    std::string outputFilePath_;
    bool recording_ = false;
    std::thread recordingThread_;
    DWORD streamIndex_;
};

extern "C" void* CreateScreenRecorder(int monitorIndex, const char* outputFilePath) {
    return static_cast<void*>(new ScreenRecorder(monitorIndex, outputFilePath));
}

extern "C" void StartRecording(void* recorder) {
    reinterpret_cast<ScreenRecorder*>(recorder)->StartRecording();
}

extern "C" void StopRecording(void* recorder) {
    reinterpret_cast<ScreenRecorder*>(recorder)->StopRecording();
}
