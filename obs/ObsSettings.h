#pragma once

#include <string>
#include <cstdint>

class CObsSettings
{
public:
    static CObsSettings& Instance();

    // Stream settings
    std::string m_server;       // RTMP server URL
    std::string m_key;          // Stream key

    // Video settings
    uint32_t m_baseWidth;
    uint32_t m_baseHeight;
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;
    uint32_t m_fpsNum;
    uint32_t m_fpsDen;

    // Audio settings
    uint32_t m_sampleRate;
    int      m_audioBitrate;

    // Encoder settings
    std::string m_videoEncoder; // "obs_x264" or "ffmpeg_nvenc"
    int         m_videoBitrate;
    std::string m_rateControl;  // "CBR", "VBR"
    int         m_keyintSec;

    // Recording settings
    std::string m_recordPath;
    std::string m_recordFormat; // "mkv", "mp4"

    void Load();
    void Save();
    void SetDefaults();

private:
    CObsSettings();
};
