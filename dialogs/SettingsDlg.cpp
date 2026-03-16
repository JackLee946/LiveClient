#include "stdafx.h"
#include "SettingsDlg.h"
#include "../obs/ObsSettings.h"
#include "../utils/StringUtil.h"

// Control IDs for each tab page (including all labels)
const UINT CSettingsDlg::s_streamIds[] = {
    IDC_STATIC_SERVER, IDC_EDIT_SERVER,
    IDC_STATIC_STREAM_KEY, IDC_EDIT_STREAM_KEY,
    0
};

const UINT CSettingsDlg::s_outputIds[] = {
    IDC_STATIC_VIDEO_ENCODER, IDC_COMBO_VIDEO_ENCODER,
    IDC_STATIC_VIDEO_BITRATE, IDC_EDIT_VIDEO_BITRATE, IDC_STATIC_VBITRATE_UNIT,
    IDC_STATIC_RATE_CONTROL, IDC_COMBO_RATE_CONTROL,
    IDC_STATIC_AUDIO_BITRATE, IDC_EDIT_AUDIO_BITRATE, IDC_STATIC_ABITRATE_UNIT,
    IDC_STATIC_RECORD_PATH, IDC_EDIT_RECORD_PATH, IDC_BTN_BROWSE_RECORD_PATH,
    IDC_STATIC_RECORD_FORMAT, IDC_COMBO_RECORD_FORMAT,
    0
};

const UINT CSettingsDlg::s_videoIds[] = {
    IDC_STATIC_BASE_RES, IDC_COMBO_BASE_RESOLUTION,
    IDC_STATIC_OUTPUT_RES, IDC_COMBO_OUTPUT_RESOLUTION,
    IDC_STATIC_FPS, IDC_COMBO_FPS,
    0
};

const UINT CSettingsDlg::s_audioIds[] = {
    IDC_STATIC_SAMPLE_RATE, IDC_COMBO_SAMPLE_RATE,
    IDC_STATIC_CHANNELS, IDC_COMBO_CHANNELS,
    0
};

CSettingsDlg::CSettingsDlg(CWnd* pParent)
    : CDialogEx(IDD_SETTINGS_DIALOG, pParent)
    , m_currentTab(TAB_STREAM)
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TAB_SETTINGS, m_tabCtrl);
}

BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_SETTINGS, &CSettingsDlg::OnTcnSelchangeTab)
    ON_BN_CLICKED(IDC_BTN_BROWSE_RECORD_PATH, &CSettingsDlg::OnBtnBrowseRecordPath)
END_MESSAGE_MAP()

BOOL CSettingsDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Setup tabs
    m_tabCtrl.InsertItem(TAB_STREAM, _T("Stream"));
    m_tabCtrl.InsertItem(TAB_OUTPUT, _T("Output"));
    m_tabCtrl.InsertItem(TAB_VIDEO, _T("Video"));
    m_tabCtrl.InsertItem(TAB_AUDIO, _T("Audio"));

    // Populate combo boxes

    // Video encoder
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_VIDEO_ENCODER);
    pCombo->AddString(_T("x264 (Software)"));
    pCombo->AddString(_T("NVENC (NVIDIA)"));

    // Rate control
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_RATE_CONTROL);
    pCombo->AddString(_T("CBR"));
    pCombo->AddString(_T("VBR"));

    // Recording format
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_RECORD_FORMAT);
    pCombo->AddString(_T("mkv"));
    pCombo->AddString(_T("mp4"));
    pCombo->AddString(_T("flv"));

    // Base resolution
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_BASE_RESOLUTION);
    pCombo->AddString(_T("1920x1080"));
    pCombo->AddString(_T("1280x720"));
    pCombo->AddString(_T("854x480"));

    // Output resolution
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_OUTPUT_RESOLUTION);
    pCombo->AddString(_T("1920x1080"));
    pCombo->AddString(_T("1280x720"));
    pCombo->AddString(_T("854x480"));

    // FPS
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_FPS);
    pCombo->AddString(_T("30"));
    pCombo->AddString(_T("60"));
    pCombo->AddString(_T("24"));
    pCombo->AddString(_T("25"));

    // Sample rate
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SAMPLE_RATE);
    pCombo->AddString(_T("48000"));
    pCombo->AddString(_T("44100"));

    // Channels
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_CHANNELS);
    pCombo->AddString(_T("Stereo"));
    pCombo->AddString(_T("Mono"));

    LoadSettings();
    ShowTabPage(TAB_STREAM);

    return TRUE;
}

void CSettingsDlg::ShowTabPage(int tab)
{
    // Hide all tab contents
    const UINT* allIds[] = { s_streamIds, s_outputIds, s_videoIds, s_audioIds };

    for (int t = 0; t < TAB_COUNT; t++)
    {
        const UINT* ids = allIds[t];
        for (int i = 0; ids[i] != 0; i++)
        {
            CWnd* pWnd = GetDlgItem(ids[i]);
            if (pWnd)
                pWnd->ShowWindow(SW_HIDE);
        }
        // Also hide static labels that are between tab control IDs
    }

    // Show selected tab contents
    const UINT* showIds = allIds[tab];
    for (int i = 0; showIds[i] != 0; i++)
    {
        CWnd* pWnd = GetDlgItem(showIds[i]);
        if (pWnd)
            pWnd->ShowWindow(SW_SHOW);
    }

    m_currentTab = tab;
}

void CSettingsDlg::OnTcnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult)
{
    UNREFERENCED_PARAMETER(pNMHDR);
    int sel = m_tabCtrl.GetCurSel();
    ShowTabPage(sel);
    *pResult = 0;
}

void CSettingsDlg::LoadSettings()
{
    CObsSettings& settings = CObsSettings::Instance();

    // Stream
    SetDlgItemText(IDC_EDIT_SERVER, StringUtil::Utf8ToWide(settings.m_server));
    SetDlgItemText(IDC_EDIT_STREAM_KEY, StringUtil::Utf8ToWide(settings.m_key));

    // Output - Video encoder
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_VIDEO_ENCODER);
    if (settings.m_videoEncoder == "ffmpeg_nvenc")
        pCombo->SetCurSel(1);
    else
        pCombo->SetCurSel(0); // x264

    // Video bitrate
    SetDlgItemInt(IDC_EDIT_VIDEO_BITRATE, settings.m_videoBitrate);

    // Rate control
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_RATE_CONTROL);
    if (settings.m_rateControl == "VBR")
        pCombo->SetCurSel(1);
    else
        pCombo->SetCurSel(0); // CBR

    // Audio bitrate
    SetDlgItemInt(IDC_EDIT_AUDIO_BITRATE, settings.m_audioBitrate);

    // Recording path
    SetDlgItemText(IDC_EDIT_RECORD_PATH, StringUtil::Utf8ToWide(settings.m_recordPath));

    // Record format
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_RECORD_FORMAT);
    if (settings.m_recordFormat == "mp4")
        pCombo->SetCurSel(1);
    else if (settings.m_recordFormat == "flv")
        pCombo->SetCurSel(2);
    else
        pCombo->SetCurSel(0); // mkv

    // Video - Base resolution
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_BASE_RESOLUTION);
    CString res;
    res.Format(_T("%dx%d"), settings.m_baseWidth, settings.m_baseHeight);
    int idx = pCombo->FindStringExact(-1, res);
    pCombo->SetCurSel(idx >= 0 ? idx : 0);

    // Output resolution
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_OUTPUT_RESOLUTION);
    res.Format(_T("%dx%d"), settings.m_outputWidth, settings.m_outputHeight);
    idx = pCombo->FindStringExact(-1, res);
    pCombo->SetCurSel(idx >= 0 ? idx : 1); // Default 720p

    // FPS
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_FPS);
    CString fps;
    fps.Format(_T("%d"), settings.m_fpsNum / (settings.m_fpsDen > 0 ? settings.m_fpsDen : 1));
    idx = pCombo->FindStringExact(-1, fps);
    pCombo->SetCurSel(idx >= 0 ? idx : 0);

    // Audio - Sample rate
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SAMPLE_RATE);
    CString sr;
    sr.Format(_T("%d"), settings.m_sampleRate);
    idx = pCombo->FindStringExact(-1, sr);
    pCombo->SetCurSel(idx >= 0 ? idx : 0);

    // Channels
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_CHANNELS);
    pCombo->SetCurSel(0); // Stereo default
}

void CSettingsDlg::SaveSettings()
{
    CObsSettings& settings = CObsSettings::Instance();

    // Stream
    CString str;
    GetDlgItemText(IDC_EDIT_SERVER, str);
    settings.m_server = StringUtil::WideToUtf8(str);
    GetDlgItemText(IDC_EDIT_STREAM_KEY, str);
    settings.m_key = StringUtil::WideToUtf8(str);

    // Video encoder
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_VIDEO_ENCODER);
    settings.m_videoEncoder = (pCombo->GetCurSel() == 1) ? "ffmpeg_nvenc" : "obs_x264";

    // Video bitrate
    settings.m_videoBitrate = GetDlgItemInt(IDC_EDIT_VIDEO_BITRATE);

    // Rate control
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_RATE_CONTROL);
    settings.m_rateControl = (pCombo->GetCurSel() == 1) ? "VBR" : "CBR";

    // Audio bitrate
    settings.m_audioBitrate = GetDlgItemInt(IDC_EDIT_AUDIO_BITRATE);

    // Record path
    GetDlgItemText(IDC_EDIT_RECORD_PATH, str);
    settings.m_recordPath = StringUtil::WideToUtf8(str);

    // Record format
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_RECORD_FORMAT);
    pCombo->GetLBText(pCombo->GetCurSel(), str);
    settings.m_recordFormat = StringUtil::WideToUtf8(str);

    // Base resolution
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_BASE_RESOLUTION);
    pCombo->GetLBText(pCombo->GetCurSel(), str);
    int w, h;
    if (_stscanf_s(str, _T("%dx%d"), &w, &h) == 2)
    {
        settings.m_baseWidth = w;
        settings.m_baseHeight = h;
    }

    // Output resolution
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_OUTPUT_RESOLUTION);
    pCombo->GetLBText(pCombo->GetCurSel(), str);
    if (_stscanf_s(str, _T("%dx%d"), &w, &h) == 2)
    {
        settings.m_outputWidth = w;
        settings.m_outputHeight = h;
    }

    // FPS
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_FPS);
    pCombo->GetLBText(pCombo->GetCurSel(), str);
    settings.m_fpsNum = _ttoi(str);
    settings.m_fpsDen = 1;

    // Sample rate
    pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SAMPLE_RATE);
    pCombo->GetLBText(pCombo->GetCurSel(), str);
    settings.m_sampleRate = _ttoi(str);

    settings.Save();
}

void CSettingsDlg::OnOK()
{
    SaveSettings();
    CDialogEx::OnOK();
}

void CSettingsDlg::OnBtnBrowseRecordPath()
{
    CFolderPickerDialog dlg(nullptr, OFN_FILEMUSTEXIST, this);
    if (dlg.DoModal() == IDOK)
    {
        SetDlgItemText(IDC_EDIT_RECORD_PATH, dlg.GetPathName());
    }
}
