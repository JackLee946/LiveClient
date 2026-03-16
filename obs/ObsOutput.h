#pragma once

#include <string>
#include <cstdint>
#include <obs.h>

class CObsEncoder;
class CObsService;

class CObsOutput
{
public:
    CObsOutput();
    ~CObsOutput();

    // Setup
    bool SetupStreamOutput(CObsEncoder* enc, CObsService* svc);
    bool SetupRecordOutput(CObsEncoder* enc, const char* filePath, const char* format = "mkv");

    // Streaming control
    bool StartStreaming();
    void StopStreaming();
    bool IsStreaming() const;

    // Recording control
    bool StartRecording();
    void StopRecording();
    bool IsRecording() const;

    // Update recording path
    void SetRecordPath(const char* filePath, const char* format = "mkv");

    // Statistics
    uint64_t GetTotalBytes() const;
    int      GetDroppedFrames() const;
    int      GetTotalFrames() const;

    // Set HWND for signal notifications (via PostMessage)
    void SetNotifyHwnd(HWND hwnd) { m_hWndNotify = hwnd; }

    // Get last error
    const std::string& GetLastError() const { return m_lastError; }

    void Release();

private:
    void ConnectStreamSignals();
    void ConnectRecordSignals();
    void DisconnectStreamSignals();
    void DisconnectRecordSignals();

    // Signal callbacks (run on OBS thread, PostMessage to UI)
    static void OnStreamStarting(void* param, calldata_t* cd);
    static void OnStreamStopping(void* param, calldata_t* cd);
    static void OnStreamStarted(void* param, calldata_t* cd);
    static void OnStreamStopped(void* param, calldata_t* cd);
    static void OnRecordStarted(void* param, calldata_t* cd);
    static void OnRecordStopped(void* param, calldata_t* cd);

    obs_output_t*  m_streamOutput;
    obs_output_t*  m_recordOutput;
    HWND           m_hWndNotify;
    std::string    m_lastError;
};
