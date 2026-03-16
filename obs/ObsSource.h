#pragma once

#include <string>
#include <obs.h>

// Source type identifiers
enum class SourceType
{
    Camera,         // dshow_input
    DisplayCapture, // monitor_capture
    WindowCapture,  // window_capture
    Image,          // image_source
    Text,           // text_ft2_source
};

class CObsSource
{
public:
    ~CObsSource();

    // Factory methods for each source type
    static CObsSource* CreateCamera(const char* name, obs_data_t* settings = nullptr);
    static CObsSource* CreateDisplayCapture(const char* name, int monitor = 0);
    static CObsSource* CreateWindowCapture(const char* name);
    static CObsSource* CreateImage(const char* name, const char* filePath = nullptr);
    static CObsSource* CreateText(const char* name, const char* text = nullptr, int fontSize = 36);

    // Update source settings
    void Update(obs_data_t* settings);

    // Properties
    obs_properties_t* GetProperties();

    // Volume control
    float GetVolume() const;
    void  SetVolume(float vol); // 0.0 to 1.0
    void  SetMuted(bool mute);
    bool  IsMuted() const;

    // Info
    obs_source_t*      GetSource() const { return m_source; }
    const std::string& GetId() const { return m_id; }
    const std::string& GetName() const { return m_name; }
    SourceType         GetType() const { return m_type; }
    bool               HasAudio() const;

    // Get current settings
    obs_data_t* GetSettings() const;

    void Release();

    // Get display name for source type
    static const char* GetTypeDisplayName(SourceType type);
    static const char* GetTypeId(SourceType type);

private:
    CObsSource();

    obs_source_t* m_source;
    std::string   m_id;
    std::string   m_name;
    SourceType    m_type;
};
