#include "stdafx.h"
#include "ObsEncoder.h"

CObsEncoder::CObsEncoder()
    : m_videoEncoder(nullptr)
    , m_audioEncoder(nullptr)
{
}

CObsEncoder::~CObsEncoder()
{
    Release();
}

bool CObsEncoder::CreateVideoEncoder(const char* id, int bitrate, int keyintSec,
                                      const char* rateControl)
{
    if (m_videoEncoder)
    {
        obs_encoder_release(m_videoEncoder);
        m_videoEncoder = nullptr;
    }

    obs_data_t* settings = obs_data_create();
    obs_data_set_int(settings, "bitrate", bitrate);
    obs_data_set_int(settings, "keyint_sec", keyintSec);
    obs_data_set_string(settings, "rate_control", rateControl);

    // x264 specific streaming settings
    if (strcmp(id, "obs_x264") == 0)
    {
        obs_data_set_string(settings, "preset", "veryfast");
        obs_data_set_string(settings, "profile", "baseline");
        obs_data_set_string(settings, "tune", "zerolatency");
    }

    m_videoEncoder = obs_video_encoder_create(id, "video_encoder", settings, nullptr);
    obs_data_release(settings);

    if (!m_videoEncoder)
        return false;

    obs_encoder_set_video(m_videoEncoder, obs_get_video());
    return true;
}

bool CObsEncoder::CreateAudioEncoder(int bitrate)
{
    if (m_audioEncoder)
    {
        obs_encoder_release(m_audioEncoder);
        m_audioEncoder = nullptr;
    }

    obs_data_t* settings = obs_data_create();
    obs_data_set_int(settings, "bitrate", bitrate);

    // Try multiple AAC encoder IDs for compatibility
    const char* aacIds[] = { "ffmpeg_aac", "mf_aac", "CoreAudio_AAC", nullptr };

    for (int i = 0; aacIds[i] != nullptr; i++)
    {
        m_audioEncoder = obs_audio_encoder_create(aacIds[i], "audio_encoder", settings, 0, nullptr);
        if (m_audioEncoder)
        {
            obs_data_release(settings);
            obs_encoder_set_audio(m_audioEncoder, obs_get_audio());
            m_lastError.clear();
            return true;
        }
    }

    obs_data_release(settings);
    m_lastError = "No AAC audio encoder available. "
                  "Tried: ffmpeg_aac, mf_aac, CoreAudio_AAC";
    return false;
}

void CObsEncoder::UpdateVideoSettings(int bitrate, int keyintSec, const char* rateControl)
{
    if (!m_videoEncoder)
        return;

    obs_data_t* settings = obs_data_create();
    obs_data_set_int(settings, "bitrate", bitrate);
    obs_data_set_int(settings, "keyint_sec", keyintSec);
    obs_data_set_string(settings, "rate_control", rateControl);

    obs_encoder_update(m_videoEncoder, settings);
    obs_data_release(settings);
}

void CObsEncoder::UpdateAudioSettings(int bitrate)
{
    if (!m_audioEncoder)
        return;

    obs_data_t* settings = obs_data_create();
    obs_data_set_int(settings, "bitrate", bitrate);

    obs_encoder_update(m_audioEncoder, settings);
    obs_data_release(settings);
}

void CObsEncoder::Release()
{
    if (m_videoEncoder)
    {
        obs_encoder_release(m_videoEncoder);
        m_videoEncoder = nullptr;
    }
    if (m_audioEncoder)
    {
        obs_encoder_release(m_audioEncoder);
        m_audioEncoder = nullptr;
    }
}
