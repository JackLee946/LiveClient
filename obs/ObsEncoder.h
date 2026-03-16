#pragma once

#include <string>
#include <obs.h>

class CObsEncoder
{
public:
    CObsEncoder();
    ~CObsEncoder();

    // Video encoder: "obs_x264" or "ffmpeg_nvenc"
    bool CreateVideoEncoder(const char* id, int bitrate, int keyintSec = 2,
                            const char* rateControl = "CBR");
    // Audio encoder: tries multiple AAC encoders for compatibility
    bool CreateAudioEncoder(int bitrate = 128);

    // Get last error
    const std::string& GetLastError() const { return m_lastError; }

    // Update settings
    void UpdateVideoSettings(int bitrate, int keyintSec = 2, const char* rateControl = "CBR");
    void UpdateAudioSettings(int bitrate);

    obs_encoder_t* GetVideoEncoder() const { return m_videoEncoder; }
    obs_encoder_t* GetAudioEncoder() const { return m_audioEncoder; }

    void Release();

private:
    obs_encoder_t* m_videoEncoder;
    obs_encoder_t* m_audioEncoder;
    std::string    m_lastError;
};
