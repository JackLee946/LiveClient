#include "stdafx.h"
#include "ObsOutput.h"
#include "ObsEncoder.h"
#include "ObsService.h"
#include "../resource.h"

CObsOutput::CObsOutput()
    : m_streamOutput(nullptr)
    , m_recordOutput(nullptr)
    , m_hWndNotify(nullptr)
{
}

CObsOutput::~CObsOutput()
{
    Release();
}

bool CObsOutput::SetupStreamOutput(CObsEncoder* enc, CObsService* svc)
{
    if (!enc || !svc)
        return false;

    if (m_streamOutput)
    {
        DisconnectStreamSignals();
        obs_output_release(m_streamOutput);
        m_streamOutput = nullptr;
    }

    m_streamOutput = obs_output_create("rtmp_output", "stream_output", nullptr, nullptr);
    if (!m_streamOutput)
    {
        m_lastError = "Failed to create RTMP output";
        return false;
    }

    // Enable low-latency network optimizations (Windows-specific)
    obs_data_t* outputSettings = obs_data_create();
    obs_data_set_bool(outputSettings, "new_socket_loop_enabled", true);
    obs_data_set_bool(outputSettings, "low_latency_mode_enabled", true);
    obs_output_update(m_streamOutput, outputSettings);
    obs_data_release(outputSettings);

    obs_output_set_video_encoder(m_streamOutput, enc->GetVideoEncoder());
    obs_output_set_audio_encoder(m_streamOutput, enc->GetAudioEncoder(), 0);
    obs_output_set_service(m_streamOutput, svc->GetService());

    // Reconnect settings: retry 10 times with 5 second delay
    obs_output_set_reconnect_settings(m_streamOutput, 10, 5);

    ConnectStreamSignals();
    return true;
}

bool CObsOutput::SetupRecordOutput(CObsEncoder* enc, const char* filePath, const char* format)
{
    if (!enc)
        return false;

    if (m_recordOutput)
    {
        DisconnectRecordSignals();
        obs_output_release(m_recordOutput);
        m_recordOutput = nullptr;
    }

    m_recordOutput = obs_output_create("ffmpeg_muxer", "record_output", nullptr, nullptr);
    if (!m_recordOutput)
    {
        m_lastError = "Failed to create recording output";
        return false;
    }

    obs_output_set_video_encoder(m_recordOutput, enc->GetVideoEncoder());
    obs_output_set_audio_encoder(m_recordOutput, enc->GetAudioEncoder(), 0);

    if (filePath)
        SetRecordPath(filePath, format);

    ConnectRecordSignals();
    return true;
}

void CObsOutput::SetRecordPath(const char* filePath, const char* format)
{
    if (!m_recordOutput)
        return;

    obs_data_t* settings = obs_data_create();
    obs_data_set_string(settings, "path", filePath);
    obs_data_set_string(settings, "muxer_settings", "");
    obs_output_update(m_recordOutput, settings);
    obs_data_release(settings);
}

bool CObsOutput::StartStreaming()
{
    if (!m_streamOutput)
    {
        m_lastError = "Stream output not initialized";
        return false;
    }

    if (obs_output_active(m_streamOutput))
        return true;

    if (!obs_output_start(m_streamOutput))
    {
        const char* err = obs_output_get_last_error(m_streamOutput);
        m_lastError = err ? err : "Unknown streaming error";
        return false;
    }

    return true;
}

void CObsOutput::StopStreaming()
{
    if (m_streamOutput && obs_output_active(m_streamOutput))
        obs_output_stop(m_streamOutput);
}

bool CObsOutput::IsStreaming() const
{
    return m_streamOutput && obs_output_active(m_streamOutput);
}

bool CObsOutput::StartRecording()
{
    if (!m_recordOutput)
    {
        m_lastError = "Record output not initialized";
        return false;
    }

    if (obs_output_active(m_recordOutput))
        return true;

    if (!obs_output_start(m_recordOutput))
    {
        const char* err = obs_output_get_last_error(m_recordOutput);
        m_lastError = err ? err : "Unknown recording error";
        return false;
    }

    return true;
}

void CObsOutput::StopRecording()
{
    if (m_recordOutput && obs_output_active(m_recordOutput))
        obs_output_stop(m_recordOutput);
}

bool CObsOutput::IsRecording() const
{
    return m_recordOutput && obs_output_active(m_recordOutput);
}

uint64_t CObsOutput::GetTotalBytes() const
{
    if (!m_streamOutput)
        return 0;
    return obs_output_get_total_bytes(m_streamOutput);
}

int CObsOutput::GetDroppedFrames() const
{
    if (!m_streamOutput)
        return 0;
    return obs_output_get_frames_dropped(m_streamOutput);
}

int CObsOutput::GetTotalFrames() const
{
    if (!m_streamOutput)
        return 0;
    return obs_output_get_total_frames(m_streamOutput);
}

void CObsOutput::Release()
{
    if (m_streamOutput)
    {
        if (obs_output_active(m_streamOutput))
            obs_output_stop(m_streamOutput);
        DisconnectStreamSignals();
        obs_output_release(m_streamOutput);
        m_streamOutput = nullptr;
    }

    if (m_recordOutput)
    {
        if (obs_output_active(m_recordOutput))
            obs_output_stop(m_recordOutput);
        DisconnectRecordSignals();
        obs_output_release(m_recordOutput);
        m_recordOutput = nullptr;
    }
}

// --- Signal handling ---

void CObsOutput::ConnectStreamSignals()
{
    if (!m_streamOutput)
        return;

    signal_handler_t* handler = obs_output_get_signal_handler(m_streamOutput);
    signal_handler_connect(handler, "start", OnStreamStarted, this);
    signal_handler_connect(handler, "stop", OnStreamStopped, this);
}

void CObsOutput::DisconnectStreamSignals()
{
    if (!m_streamOutput)
        return;

    signal_handler_t* handler = obs_output_get_signal_handler(m_streamOutput);
    signal_handler_disconnect(handler, "start", OnStreamStarted, this);
    signal_handler_disconnect(handler, "stop", OnStreamStopped, this);
}

void CObsOutput::ConnectRecordSignals()
{
    if (!m_recordOutput)
        return;

    signal_handler_t* handler = obs_output_get_signal_handler(m_recordOutput);
    signal_handler_connect(handler, "start", OnRecordStarted, this);
    signal_handler_connect(handler, "stop", OnRecordStopped, this);
}

void CObsOutput::DisconnectRecordSignals()
{
    if (!m_recordOutput)
        return;

    signal_handler_t* handler = obs_output_get_signal_handler(m_recordOutput);
    signal_handler_disconnect(handler, "start", OnRecordStarted, this);
    signal_handler_disconnect(handler, "stop", OnRecordStopped, this);
}

// Signal callbacks - these run on OBS internal threads
// We use PostMessage to safely notify the MFC UI thread

void CObsOutput::OnStreamStarting(void* param, calldata_t* cd)
{
    UNREFERENCED_PARAMETER(cd);
}

void CObsOutput::OnStreamStopping(void* param, calldata_t* cd)
{
    UNREFERENCED_PARAMETER(cd);
}

void CObsOutput::OnStreamStarted(void* param, calldata_t* cd)
{
    UNREFERENCED_PARAMETER(cd);
    CObsOutput* self = static_cast<CObsOutput*>(param);
    if (self->m_hWndNotify)
        ::PostMessage(self->m_hWndNotify, WM_OBS_STREAM_STARTED, 0, 0);
}

void CObsOutput::OnStreamStopped(void* param, calldata_t* cd)
{
    CObsOutput* self = static_cast<CObsOutput*>(param);

    // Check if stopped due to error
    int code = (int)calldata_int(cd, "code");

    if (code != OBS_OUTPUT_SUCCESS)
    {
        // Capture the OBS error message while still on the OBS thread
        const char* err = obs_output_get_last_error(self->m_streamOutput);
        self->m_lastError = err ? err : "";
    }

    if (self->m_hWndNotify)
    {
        if (code != OBS_OUTPUT_SUCCESS)
            ::PostMessage(self->m_hWndNotify, WM_OBS_STREAM_ERROR, (WPARAM)code, 0);
        else
            ::PostMessage(self->m_hWndNotify, WM_OBS_STREAM_STOPPED, 0, 0);
    }
}

void CObsOutput::OnRecordStarted(void* param, calldata_t* cd)
{
    UNREFERENCED_PARAMETER(cd);
    CObsOutput* self = static_cast<CObsOutput*>(param);
    if (self->m_hWndNotify)
        ::PostMessage(self->m_hWndNotify, WM_OBS_RECORD_STARTED, 0, 0);
}

void CObsOutput::OnRecordStopped(void* param, calldata_t* cd)
{
    UNREFERENCED_PARAMETER(cd);
    CObsOutput* self = static_cast<CObsOutput*>(param);
    if (self->m_hWndNotify)
        ::PostMessage(self->m_hWndNotify, WM_OBS_RECORD_STOPPED, 0, 0);
}
