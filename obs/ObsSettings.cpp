#include "stdafx.h"
#include "ObsSettings.h"
#include "../utils/StringUtil.h"

CObsSettings::CObsSettings()
{
    SetDefaults();
}

CObsSettings& CObsSettings::Instance()
{
    static CObsSettings instance;
    return instance;
}

void CObsSettings::SetDefaults()
{
    m_server       = "";
    m_key          = "";

    m_baseWidth    = 1920;
    m_baseHeight   = 1080;
    m_outputWidth  = 1280;
    m_outputHeight = 720;
    m_fpsNum       = 30;
    m_fpsDen       = 1;

    m_sampleRate   = 48000;
    m_audioBitrate = 128;

    m_videoEncoder = "obs_x264";
    m_videoBitrate = 2500;
    m_rateControl  = "CBR";
    m_keyintSec    = 2;

    m_recordPath   = "";
    m_recordFormat = "mkv";
}

void CObsSettings::Load()
{
    CWinApp* pApp = AfxGetApp();
    if (!pApp)
    {
        SetDefaults();
        return;
    }

    m_server       = StringUtil::WideToUtf8(pApp->GetProfileString(_T("Stream"), _T("Server"), _T("")));
    m_key          = StringUtil::WideToUtf8(pApp->GetProfileString(_T("Stream"), _T("Key"), _T("")));

    m_baseWidth    = pApp->GetProfileInt(_T("Video"), _T("BaseWidth"), 1920);
    m_baseHeight   = pApp->GetProfileInt(_T("Video"), _T("BaseHeight"), 1080);
    m_outputWidth  = pApp->GetProfileInt(_T("Video"), _T("OutputWidth"), 1280);
    m_outputHeight = pApp->GetProfileInt(_T("Video"), _T("OutputHeight"), 720);
    m_fpsNum       = pApp->GetProfileInt(_T("Video"), _T("FPSNum"), 30);
    m_fpsDen       = pApp->GetProfileInt(_T("Video"), _T("FPSDen"), 1);

    m_sampleRate   = pApp->GetProfileInt(_T("Audio"), _T("SampleRate"), 48000);
    m_audioBitrate = pApp->GetProfileInt(_T("Audio"), _T("Bitrate"), 128);

    m_videoEncoder = StringUtil::WideToUtf8(pApp->GetProfileString(_T("Encoder"), _T("Video"), _T("obs_x264")));
    m_videoBitrate = pApp->GetProfileInt(_T("Encoder"), _T("VideoBitrate"), 2500);
    m_rateControl  = StringUtil::WideToUtf8(pApp->GetProfileString(_T("Encoder"), _T("RateControl"), _T("CBR")));
    m_keyintSec    = pApp->GetProfileInt(_T("Encoder"), _T("KeyintSec"), 2);

    m_recordPath   = StringUtil::WideToUtf8(pApp->GetProfileString(_T("Record"), _T("Path"), _T("")));
    m_recordFormat = StringUtil::WideToUtf8(pApp->GetProfileString(_T("Record"), _T("Format"), _T("mkv")));

    // Use defaults for empty record path
    if (m_recordPath.empty())
    {
        TCHAR szPath[MAX_PATH];
        if (SHGetFolderPath(NULL, CSIDL_MYVIDEO, NULL, 0, szPath) == S_OK)
            m_recordPath = StringUtil::WideToUtf8(szPath);
    }
}

void CObsSettings::Save()
{
    CWinApp* pApp = AfxGetApp();
    if (!pApp)
        return;

    pApp->WriteProfileString(_T("Stream"), _T("Server"), StringUtil::Utf8ToWide(m_server));
    pApp->WriteProfileString(_T("Stream"), _T("Key"), StringUtil::Utf8ToWide(m_key));

    pApp->WriteProfileInt(_T("Video"), _T("BaseWidth"), m_baseWidth);
    pApp->WriteProfileInt(_T("Video"), _T("BaseHeight"), m_baseHeight);
    pApp->WriteProfileInt(_T("Video"), _T("OutputWidth"), m_outputWidth);
    pApp->WriteProfileInt(_T("Video"), _T("OutputHeight"), m_outputHeight);
    pApp->WriteProfileInt(_T("Video"), _T("FPSNum"), m_fpsNum);
    pApp->WriteProfileInt(_T("Video"), _T("FPSDen"), m_fpsDen);

    pApp->WriteProfileInt(_T("Audio"), _T("SampleRate"), m_sampleRate);
    pApp->WriteProfileInt(_T("Audio"), _T("Bitrate"), m_audioBitrate);

    pApp->WriteProfileString(_T("Encoder"), _T("Video"), StringUtil::Utf8ToWide(m_videoEncoder));
    pApp->WriteProfileInt(_T("Encoder"), _T("VideoBitrate"), m_videoBitrate);
    pApp->WriteProfileString(_T("Encoder"), _T("RateControl"), StringUtil::Utf8ToWide(m_rateControl));
    pApp->WriteProfileInt(_T("Encoder"), _T("KeyintSec"), m_keyintSec);

    pApp->WriteProfileString(_T("Record"), _T("Path"), StringUtil::Utf8ToWide(m_recordPath));
    pApp->WriteProfileString(_T("Record"), _T("Format"), StringUtil::Utf8ToWide(m_recordFormat));
}
