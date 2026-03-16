#pragma once

#include <string>
#include <cstdint>

class CObsSettings;

class CObsApp
{
public:
    static CObsApp& Instance();

    bool Initialize(const CObsSettings& settings);
    void Shutdown();

    bool ResetVideo(uint32_t baseW, uint32_t baseH,
                    uint32_t outW, uint32_t outH,
                    uint32_t fpsNum, uint32_t fpsDen);
    bool ResetAudio(uint32_t sampleRate);

    bool IsInitialized() const { return m_bInitialized; }
    const std::string& GetLastError() const { return m_lastError; }

private:
    CObsApp();
    ~CObsApp();

    void LoadModules();

    bool        m_bInitialized;
    std::string m_lastError;
};
