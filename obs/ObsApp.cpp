#include "stdafx.h"
#include "ObsApp.h"
#include "ObsSettings.h"
#include "../utils/StringUtil.h"
#include "../filter/BeautyFilter.h"

// Get directory containing the running executable
static std::string GetExeDirectory()
{
    TCHAR szExePath[MAX_PATH];
    GetModuleFileName(NULL, szExePath, MAX_PATH);
    CString exeDir(szExePath);
    int pos = exeDir.ReverseFind(_T('\\'));
    if (pos >= 0)
        exeDir = exeDir.Left(pos);
    return StringUtil::WideToUtf8(exeDir);
}

CObsApp::CObsApp()
    : m_bInitialized(false)
{
}

CObsApp::~CObsApp()
{
    Shutdown();
}

CObsApp& CObsApp::Instance()
{
    static CObsApp instance;
    return instance;
}

bool CObsApp::Initialize(const CObsSettings& settings)
{
    if (m_bInitialized)
        return true;

    std::string exeDir = GetExeDirectory();

    // Verify that obs.dll and key files exist
    std::string obsDll = exeDir + "\\obs.dll";
    std::string d3d11Dll = exeDir + "\\libobs-d3d11.dll";
    std::string effectDir = exeDir + "\\data\\libobs";

    if (GetFileAttributesA(obsDll.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        m_lastError = "obs.dll not found at: " + obsDll +
            "\nMake sure Post-Build event has copied OBS runtime files.";
        return false;
    }
    if (GetFileAttributesA(d3d11Dll.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        m_lastError = "libobs-d3d11.dll not found at: " + d3d11Dll;
        return false;
    }
    if (GetFileAttributesA(effectDir.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        m_lastError = "libobs data directory not found at: " + effectDir +
            "\nEnsure data/libobs/ is copied to output directory.";
        return false;
    }

    // Initialize OBS core
    if (!obs_startup("en-US", nullptr, nullptr))
    {
        m_lastError = "obs_startup() failed.";
        return false;
    }

    // Set libobs data path BEFORE obs_reset_video
    // OBS needs effect files (default.effect, format_conversion.effect, etc.)
    std::string libobsDataPath = exeDir + "/data/libobs/";
    obs_add_data_path(libobsDataPath.c_str());

    // Also set the general data path
    std::string dataPath = exeDir + "/data/";
    obs_add_data_path(dataPath.c_str());

    // Reset video subsystem
    if (!ResetVideo(settings.m_baseWidth, settings.m_baseHeight,
                    settings.m_outputWidth, settings.m_outputHeight,
                    settings.m_fpsNum, settings.m_fpsDen))
    {
        m_lastError = "obs_reset_video() failed.\n"
            "Check that libobs-d3d11.dll is accessible and "
            "data/libobs/*.effect files exist.";
        obs_shutdown();
        return false;
    }

    // Reset audio subsystem
    if (!ResetAudio(settings.m_sampleRate))
    {
        m_lastError = "obs_reset_audio() failed.";
        obs_shutdown();
        return false;
    }

    // Register custom filters before loading modules
    RegisterBeautyFilter();

    // Load OBS modules (plugins)
    LoadModules();

    m_bInitialized = true;
    return true;
}

void CObsApp::Shutdown()
{
    if (!m_bInitialized)
        return;

    obs_shutdown();
    m_bInitialized = false;
}

bool CObsApp::ResetVideo(uint32_t baseW, uint32_t baseH,
                          uint32_t outW, uint32_t outH,
                          uint32_t fpsNum, uint32_t fpsDen)
{
    struct obs_video_info ovi = {};
    ovi.graphics_module = "libobs-d3d11";
    ovi.fps_num         = fpsNum;
    ovi.fps_den         = fpsDen;
    ovi.base_width      = baseW;
    ovi.base_height     = baseH;
    ovi.output_width    = outW;
    ovi.output_height   = outH;
    ovi.output_format   = VIDEO_FORMAT_NV12;
    ovi.adapter         = 0;
    ovi.gpu_conversion  = true;
    ovi.colorspace      = VIDEO_CS_709;
    ovi.range           = VIDEO_RANGE_PARTIAL;
    ovi.scale_type      = OBS_SCALE_BICUBIC;

    int ret = obs_reset_video(&ovi);
    return (ret == OBS_VIDEO_SUCCESS);
}

bool CObsApp::ResetAudio(uint32_t sampleRate)
{
    struct obs_audio_info ai = {};
    ai.samples_per_sec = sampleRate;
    ai.speakers        = SPEAKERS_STEREO;

    return obs_reset_audio(&ai);
}

void CObsApp::LoadModules()
{
    std::string exeDir = GetExeDirectory();

    // Plugin binaries
    std::string pluginBinPath = exeDir + "/obs-plugins/32bit";
    // Plugin data
    std::string pluginDataPath = exeDir + "/data/obs-plugins/%module%";

    obs_add_module_path(pluginBinPath.c_str(), pluginDataPath.c_str());

    obs_load_all_modules();
    obs_post_load_modules();
}
