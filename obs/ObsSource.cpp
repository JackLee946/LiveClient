#include "stdafx.h"
#include "ObsSource.h"

CObsSource::CObsSource()
    : m_source(nullptr)
    , m_type(SourceType::Camera)
{
}

CObsSource::~CObsSource()
{
    Release();
}

void CObsSource::Release()
{
    if (m_source)
    {
        obs_source_release(m_source);
        m_source = nullptr;
    }
}

const char* CObsSource::GetTypeDisplayName(SourceType type)
{
    switch (type)
    {
    case SourceType::Camera:         return "Camera";
    case SourceType::DisplayCapture: return "Display Capture";
    case SourceType::WindowCapture:  return "Window Capture";
    case SourceType::Image:          return "Image";
    case SourceType::Text:           return "Text";
    default:                         return "Unknown";
    }
}

const char* CObsSource::GetTypeId(SourceType type)
{
    switch (type)
    {
    case SourceType::Camera:         return "dshow_input";
    case SourceType::DisplayCapture: return "monitor_capture";
    case SourceType::WindowCapture:  return "window_capture";
    case SourceType::Image:          return "image_source";
    case SourceType::Text:           return "text_ft2_source";
    default:                         return "";
    }
}

CObsSource* CObsSource::CreateCamera(const char* name, obs_data_t* settings)
{
    obs_data_t* s = settings;
    bool ownSettings = false;
    if (!s)
    {
        s = obs_data_create();
        ownSettings = true;

        // Default camera settings for reliable streaming
        // fps_type: 0=highest fps, 1=match output fps, 2=custom
        obs_data_set_int(s, "res_type", 0);      // 0 = device preferred resolution
        obs_data_set_int(s, "fps_type", 1);       // 1 = match OBS output fps
        obs_data_set_bool(s, "active", true);
    }

    obs_source_t* source = obs_source_create("dshow_input", name, s, nullptr);
    if (ownSettings)
        obs_data_release(s);

    if (!source)
        return nullptr;

    // If no video_device_id was specified, auto-select the first available camera
    obs_data_t* curSettings = obs_source_get_settings(source);
    const char* deviceId = obs_data_get_string(curSettings, "video_device_id");
    bool needAutoSelect = (!deviceId || deviceId[0] == '\0');
    obs_data_release(curSettings);

    if (needAutoSelect)
    {
        obs_properties_t* props = obs_source_properties(source);
        if (props)
        {
            obs_property_t* prop = obs_properties_get(props, "video_device_id");
            if (prop && obs_property_list_item_count(prop) > 0)
            {
                const char* firstDevice = obs_property_list_item_string(prop, 0);
                if (firstDevice && firstDevice[0] != '\0')
                {
                    obs_data_t* update = obs_data_create();
                    obs_data_set_string(update, "video_device_id", firstDevice);
                    obs_data_set_int(update, "res_type", 0);
                    obs_data_set_int(update, "fps_type", 1);
                    obs_data_set_bool(update, "active", true);
                    obs_source_update(source, update);
                    obs_data_release(update);
                }
            }
            obs_properties_destroy(props);
        }
    }

    CObsSource* obj = new CObsSource();
    obj->m_source = source;
    obj->m_id     = "dshow_input";
    obj->m_name   = name;
    obj->m_type   = SourceType::Camera;

    // Auto-attach beauty filter to camera source
    obs_source_t* beautyFilter = obs_source_create(
        "beauty_filter", "Beauty", nullptr, nullptr);
    if (beautyFilter)
    {
        obs_source_filter_add(source, beautyFilter);
        obs_source_release(beautyFilter);
    }

    return obj;
}

CObsSource* CObsSource::CreateDisplayCapture(const char* name, int monitor)
{
    obs_data_t* settings = obs_data_create();
    obs_data_set_int(settings, "monitor", monitor);

    obs_source_t* source = obs_source_create("monitor_capture", name, settings, nullptr);
    obs_data_release(settings);

    if (!source)
        return nullptr;

    CObsSource* obj = new CObsSource();
    obj->m_source = source;
    obj->m_id     = "monitor_capture";
    obj->m_name   = name;
    obj->m_type   = SourceType::DisplayCapture;
    return obj;
}

CObsSource* CObsSource::CreateWindowCapture(const char* name)
{
    obs_data_t* settings = obs_data_create();

    obs_source_t* source = obs_source_create("window_capture", name, settings, nullptr);
    obs_data_release(settings);

    if (!source)
        return nullptr;

    CObsSource* obj = new CObsSource();
    obj->m_source = source;
    obj->m_id     = "window_capture";
    obj->m_name   = name;
    obj->m_type   = SourceType::WindowCapture;
    return obj;
}

CObsSource* CObsSource::CreateImage(const char* name, const char* filePath)
{
    obs_data_t* settings = obs_data_create();
    if (filePath)
        obs_data_set_string(settings, "file", filePath);

    obs_source_t* source = obs_source_create("image_source", name, settings, nullptr);
    obs_data_release(settings);

    if (!source)
        return nullptr;

    CObsSource* obj = new CObsSource();
    obj->m_source = source;
    obj->m_id     = "image_source";
    obj->m_name   = name;
    obj->m_type   = SourceType::Image;
    return obj;
}

CObsSource* CObsSource::CreateText(const char* name, const char* text, int fontSize)
{
    obs_data_t* settings = obs_data_create();
    if (text)
        obs_data_set_string(settings, "text", text);

    // Create font object
    obs_data_t* font = obs_data_create();
    obs_data_set_string(font, "face", "Arial");
    obs_data_set_int(font, "size", fontSize);
    obs_data_set_obj(settings, "font", font);
    obs_data_release(font);

    obs_source_t* source = obs_source_create("text_ft2_source", name, settings, nullptr);
    obs_data_release(settings);

    if (!source)
        return nullptr;

    CObsSource* obj = new CObsSource();
    obj->m_source = source;
    obj->m_id     = "text_ft2_source";
    obj->m_name   = name;
    obj->m_type   = SourceType::Text;
    return obj;
}

void CObsSource::Update(obs_data_t* settings)
{
    if (m_source && settings)
        obs_source_update(m_source, settings);
}

obs_properties_t* CObsSource::GetProperties()
{
    if (!m_source)
        return nullptr;
    return obs_source_properties(m_source);
}

float CObsSource::GetVolume() const
{
    if (!m_source)
        return 0.0f;
    return obs_source_get_volume(m_source);
}

void CObsSource::SetVolume(float vol)
{
    if (m_source)
        obs_source_set_volume(m_source, vol);
}

void CObsSource::SetMuted(bool mute)
{
    if (m_source)
        obs_source_set_muted(m_source, mute);
}

bool CObsSource::IsMuted() const
{
    if (!m_source)
        return false;
    return obs_source_muted(m_source);
}

bool CObsSource::HasAudio() const
{
    if (!m_source)
        return false;
    uint32_t flags = obs_source_get_output_flags(m_source);
    return (flags & OBS_SOURCE_AUDIO) != 0;
}

obs_data_t* CObsSource::GetSettings() const
{
    if (!m_source)
        return nullptr;
    return obs_source_get_settings(m_source);
}
