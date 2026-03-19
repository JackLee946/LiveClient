#include "stdafx.h"
#include "LiveClientDlg.h"
#include "AddSourceDlg.h"
#include "SourcePropDlg.h"
#include "SettingsDlg.h"
#include "../LiveClient.h"
#include "../obs/ObsApp.h"
#include "../obs/ObsSettings.h"
#include "../utils/StringUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CLiveClientDlg::CLiveClientDlg(CWnd* pParent)
    : CDialogEx(IDD_LIVECLIENT_DIALOG, pParent)
    , m_bStreaming(false)
    , m_bRecording(false)
    , m_dwStreamStartTime(0)
    , m_dwRecordStartTime(0)
    , m_lastTotalBytes(0)
    , m_statusColor(RGB(128, 128, 128))
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLiveClientDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_PREVIEW, m_staticPreview);
    DDX_Control(pDX, IDC_LIST_SCENES, m_listScenes);
    DDX_Control(pDX, IDC_LIST_SOURCES, m_listSources);
    DDX_Control(pDX, IDC_SLIDER_MASTER, m_sliderMaster);
    DDX_Control(pDX, IDC_CHECK_MUTE_MASTER, m_checkMuteMaster);
    DDX_Control(pDX, IDC_CHECK_BEAUTY, m_checkBeauty);
    DDX_Control(pDX, IDC_SLIDER_SMOOTH, m_sliderSmooth);
    DDX_Control(pDX, IDC_SLIDER_WHITE, m_sliderWhite);
}

BEGIN_MESSAGE_MAP(CLiveClientDlg, CDialogEx)
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_TIMER()
    ON_WM_CLOSE()
    ON_WM_DESTROY()
    ON_WM_CTLCOLOR()
    ON_WM_HSCROLL()

    // Scene buttons
    ON_BN_CLICKED(IDC_BTN_ADD_SCENE, &CLiveClientDlg::OnBtnAddScene)
    ON_BN_CLICKED(IDC_BTN_REMOVE_SCENE, &CLiveClientDlg::OnBtnRemoveScene)
    ON_LBN_SELCHANGE(IDC_LIST_SCENES, &CLiveClientDlg::OnSceneSelChange)

    // Source buttons
    ON_BN_CLICKED(IDC_BTN_ADD_SOURCE, &CLiveClientDlg::OnBtnAddSource)
    ON_BN_CLICKED(IDC_BTN_REMOVE_SOURCE, &CLiveClientDlg::OnBtnRemoveSource)
    ON_BN_CLICKED(IDC_BTN_SOURCE_PROPS, &CLiveClientDlg::OnBtnSourceProps)
    ON_BN_CLICKED(IDC_BTN_SOURCE_UP, &CLiveClientDlg::OnBtnSourceUp)
    ON_BN_CLICKED(IDC_BTN_SOURCE_DOWN, &CLiveClientDlg::OnBtnSourceDown)

    // Audio
    ON_BN_CLICKED(IDC_CHECK_MUTE_MASTER, &CLiveClientDlg::OnCheckMuteMaster)

    // Beauty
    ON_BN_CLICKED(IDC_CHECK_BEAUTY, &CLiveClientDlg::OnCheckBeauty)

    // Control buttons
    ON_BN_CLICKED(IDC_BTN_START_STREAM, &CLiveClientDlg::OnBtnStartStream)
    ON_BN_CLICKED(IDC_BTN_START_RECORD, &CLiveClientDlg::OnBtnStartRecord)
    ON_BN_CLICKED(IDC_BTN_SETTINGS, &CLiveClientDlg::OnBtnSettings)

    // OBS signals
    ON_MESSAGE(WM_OBS_STREAM_STARTED, &CLiveClientDlg::OnObsStreamStarted)
    ON_MESSAGE(WM_OBS_STREAM_STOPPED, &CLiveClientDlg::OnObsStreamStopped)
    ON_MESSAGE(WM_OBS_STREAM_ERROR, &CLiveClientDlg::OnObsStreamError)
    ON_MESSAGE(WM_OBS_RECORD_STARTED, &CLiveClientDlg::OnObsRecordStarted)
    ON_MESSAGE(WM_OBS_RECORD_STOPPED, &CLiveClientDlg::OnObsRecordStopped)
END_MESSAGE_MAP()

BOOL CLiveClientDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // Setup source list columns
    m_listSources.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
    m_listSources.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100);
    m_listSources.InsertColumn(1, _T("Type"), LVCFMT_LEFT, 68);

    // Setup master volume slider
    m_sliderMaster.SetRange(0, 100);
    m_sliderMaster.SetPos(100);

    // Setup beauty sliders
    m_sliderSmooth.SetRange(0, 10);
    m_sliderSmooth.SetPos(5);
    m_sliderWhite.SetRange(0, 10);
    m_sliderWhite.SetPos(3);
    m_checkBeauty.SetCheck(BST_UNCHECKED);

    // Setup OBS components
    SetupObs();

    // Start status timer
    SetTimer(ID_TIMER_STATUS, 1000, nullptr);

    // Initial layout
    CRect rc;
    GetClientRect(&rc);
    LayoutControls(rc.Width(), rc.Height());

    return TRUE;
}

void CLiveClientDlg::SetupObs()
{
    CObsSettings& settings = CObsSettings::Instance();

    // Create default scene
    CObsScene* scene = new CObsScene();
    if (scene->Create("Scene 1"))
    {
        m_scenes.push_back(scene);
        scene->SetAsCurrent();
        m_listScenes.AddString(_T("Scene 1"));
        m_listScenes.SetCurSel(0);
    }
    else
    {
        delete scene;
    }

    // Create preview display
    CRect rcPreview;
    m_staticPreview.GetClientRect(&rcPreview);
    m_preview.Create(m_staticPreview.GetSafeHwnd(),
                     rcPreview.Width(), rcPreview.Height());

    // Create encoders
    if (!m_encoder.CreateVideoEncoder(settings.m_videoEncoder.c_str(),
                                  settings.m_videoBitrate,
                                  settings.m_keyintSec,
                                  settings.m_rateControl.c_str()))
    {
        AfxMessageBox(_T("Failed to create video encoder.\n")
                      _T("Check that the obs-x264 plugin is loaded."), MB_ICONWARNING);
    }

    if (!m_encoder.CreateAudioEncoder(settings.m_audioBitrate))
    {
        CString msg = _T("Failed to create audio encoder.\n");
        msg += StringUtil::Utf8ToWide(m_encoder.GetLastError());
        AfxMessageBox(msg, MB_ICONWARNING);
    }

    // Create RTMP service
    m_service.Create(settings.m_server.c_str(), settings.m_key.c_str());

    // Setup outputs
    m_output.SetNotifyHwnd(GetSafeHwnd());
    m_output.SetupStreamOutput(&m_encoder, &m_service);

    // Setup recording output
    if (!settings.m_recordPath.empty())
    {
        // Generate default recording file path
        SYSTEMTIME st;
        GetLocalTime(&st);
        char filePath[MAX_PATH];
        sprintf_s(filePath, "%s\\%04d-%02d-%02d_%02d-%02d-%02d.%s",
                  settings.m_recordPath.c_str(),
                  st.wYear, st.wMonth, st.wDay,
                  st.wHour, st.wMinute, st.wSecond,
                  settings.m_recordFormat.c_str());
        m_output.SetupRecordOutput(&m_encoder, filePath, settings.m_recordFormat.c_str());
    }
}

void CLiveClientDlg::ShutdownObs()
{
    // Stop active outputs first
    m_output.Release();

    // Destroy preview
    m_preview.Destroy();

    // Release all sources
    for (auto* src : m_sources)
        delete src;
    m_sources.clear();

    // Release all scenes
    for (auto* scene : m_scenes)
        delete scene;
    m_scenes.clear();

    // Release encoder and service
    m_encoder.Release();
    m_service.Release();
}

void CLiveClientDlg::OnClose()
{
    KillTimer(ID_TIMER_STATUS);

    // Stop streaming/recording if active
    if (m_bStreaming)
        m_output.StopStreaming();
    if (m_bRecording)
        m_output.StopRecording();

    ShutdownObs();
    CDialogEx::OnClose();
}

void CLiveClientDlg::OnDestroy()
{
    CDialogEx::OnDestroy();
}

void CLiveClientDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    lpMMI->ptMinTrackSize.x = 800;
    lpMMI->ptMinTrackSize.y = 550;
    CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CLiveClientDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0 && m_staticPreview.GetSafeHwnd())
        LayoutControls(cx, cy);
}

void CLiveClientDlg::LayoutControls(int cx, int cy)
{
    const int margin = 10;           // slightly larger margin
    const int gap = 8;               // slightly larger gap
    const int rightPanelW = 240;     // slightly wider for buttons
    const int statusH = 20;
    const int bottomRowH = 75;       // taller bottom row
    const int btnH = 24;             // standard button height

    // ---- Status bar (very bottom) ----
    int statusY = cy - margin - statusH;
    GetDlgItem(IDC_STATIC_STATUS)->MoveWindow(margin, statusY, cx - margin * 2 - 130, statusH);
    GetDlgItem(IDC_STATIC_STREAM_TIME)->MoveWindow(cx - margin - 125, statusY, 125, statusH);

    // ---- Bottom row: Mixer (left) + Beauty (center) + Controls (right) ----
    int bottomY = statusY - gap - bottomRowH;

    // Centering constants for bottom row (consistent alignment)
    int rowCenterY = bottomY + (bottomRowH + 10) / 2; // offset for groupbox title
    int labelH = 20; // Increased from 14 to 20 to fix vertical text clipping
    int labelY = rowCenterY - (labelH / 2);
    int sliderY = rowCenterY - 11;

    // Audio Mixer group (left part of bottom row)
    int mixerW = (cx - margin * 2 - rightPanelW - gap) / 2;
    GetDlgItem(IDC_GROUP_MIXER)->MoveWindow(margin, bottomY, mixerW, bottomRowH);
    int mLabelW = 85; // Extra large width for high DPI fonts
    GetDlgItem(IDC_STATIC_MASTER_LABEL)->MoveWindow(margin + 12, labelY, mLabelW, labelH);
    int sliderX = margin + 12 + mLabelW + 5;
    int mMuteW = 80;  // Extra large for Mute checkbox
    int sliderW = mixerW - (12 + mLabelW + 5 + mMuteW);
    if (sliderW < 50) sliderW = 50;
    GetDlgItem(IDC_SLIDER_MASTER)->MoveWindow(sliderX, sliderY, sliderW, 22);
    GetDlgItem(IDC_CHECK_MUTE_MASTER)->MoveWindow(sliderX + sliderW + 5, labelY, mMuteW, labelH);

    // Beauty group (middle of bottom row, split into 2 rows to avoid scaling overlap)
    int beautyX = margin + mixerW + gap;
    int beautyW = cx - margin * 2 - rightPanelW - gap - mixerW - gap;
    GetDlgItem(IDC_GROUP_BEAUTY)->MoveWindow(beautyX, bottomY, beautyW, bottomRowH);

    // Row 1 (Enable Beauty Checkbox)
    int bRow1Y = bottomY + 18;
    GetDlgItem(IDC_CHECK_BEAUTY)->MoveWindow(beautyX + 12, bRow1Y, 150, labelH); // Give Enable massive width and accurate height

    // Row 2 (Smooth and White)
    int bRow2Y = bottomY + 44;
    int bLabelW = 85; 
    int halfW = (beautyW - 20) / 2; // Split remaining row 2 into halves

    // Left half (Smooth)
    GetDlgItem(IDC_STATIC_SMOOTH_LABEL)->MoveWindow(beautyX + 12, bRow2Y, bLabelW, labelH);
    int sSliderW = halfW - bLabelW - 5;
    if (sSliderW < 40) sSliderW = 40;
    GetDlgItem(IDC_SLIDER_SMOOTH)->MoveWindow(beautyX + 12 + bLabelW, bRow2Y - 1, sSliderW, 22);
    
    // Right half (White)
    int whiteX = beautyX + 10 + halfW;
    GetDlgItem(IDC_STATIC_WHITE_LABEL)->MoveWindow(whiteX + 5, bRow2Y, bLabelW, labelH);
    int wSliderW = halfW - bLabelW - 10;
    if (wSliderW < 40) wSliderW = 40;
    GetDlgItem(IDC_SLIDER_WHITE)->MoveWindow(whiteX + 5 + bLabelW, bRow2Y - 1, wSliderW, 22);

    // Controls group (right part of bottom row)
    int ctrlX = cx - margin - rightPanelW;
    GetDlgItem(IDC_GROUP_CONTROLS)->MoveWindow(ctrlX, bottomY, rightPanelW, bottomRowH);
    int cbtnW = (rightPanelW - 25) / 2;
    int cbtnH = 26; // Increased button height from 24 to 26 for high-DPI
    int cbtnY = bottomY + 20;
    GetDlgItem(IDC_BTN_START_STREAM)->MoveWindow(ctrlX + 10, cbtnY, cbtnW, cbtnH);
    GetDlgItem(IDC_BTN_START_RECORD)->MoveWindow(ctrlX + 10 + cbtnW + 5, cbtnY, cbtnW, cbtnH);
    int settingsW = 100;
    GetDlgItem(IDC_BTN_SETTINGS)->MoveWindow(ctrlX + (rightPanelW - settingsW) / 2, bottomY + 48, settingsW, cbtnH);

    // ---- Preview area (left, fills remaining space) ----
    int previewW = cx - margin * 2 - rightPanelW - gap;
    int previewH = bottomY - gap - margin;
    m_staticPreview.MoveWindow(margin, margin, previewW, previewH);

    if (m_preview.IsCreated())
        m_preview.Resize(previewW, previewH);

    // ---- Right panel: Scenes (top 40%) + Sources (bottom 60%) ----
    int rightX = cx - margin - rightPanelW;
    int rightH = bottomY - gap - margin;
    int sceneH = (int)(rightH * 0.38);
    int sourceH = rightH - sceneH - gap;
    int listW = rightPanelW - 20;
    int listInset = 10;
    int smallBtnW = 40;
    int smallBtnH = 24;

    // Scene group
    GetDlgItem(IDC_GROUP_SCENES)->MoveWindow(rightX, margin, rightPanelW, sceneH);
    m_listScenes.MoveWindow(rightX + listInset, margin + 18, listW, sceneH - 44);
    int sceneBtnY = margin + sceneH - smallBtnH - 4;
    GetDlgItem(IDC_BTN_ADD_SCENE)->MoveWindow(rightX + listInset, sceneBtnY, smallBtnW, smallBtnH);
    GetDlgItem(IDC_BTN_REMOVE_SCENE)->MoveWindow(rightX + listInset + smallBtnW + 4, sceneBtnY, smallBtnW, smallBtnH);

    // Source group
    int srcY = margin + sceneH + gap;
    GetDlgItem(IDC_GROUP_SOURCES)->MoveWindow(rightX, srcY, rightPanelW, sourceH);
    m_listSources.MoveWindow(rightX + listInset, srcY + 18, listW, sourceH - 44);
    int srcBtnY = srcY + sourceH - smallBtnH - 4;
    int srcBtnGap = 4;
    int bx = rightX + listInset;
    GetDlgItem(IDC_BTN_ADD_SOURCE)->MoveWindow(bx, srcBtnY, smallBtnW, smallBtnH);
    bx += smallBtnW + srcBtnGap;
    GetDlgItem(IDC_BTN_REMOVE_SOURCE)->MoveWindow(bx, srcBtnY, smallBtnW, smallBtnH);
    bx += smallBtnW + srcBtnGap;
    GetDlgItem(IDC_BTN_SOURCE_PROPS)->MoveWindow(bx, srcBtnY, smallBtnW, smallBtnH);
    bx += smallBtnW + srcBtnGap;
    GetDlgItem(IDC_BTN_SOURCE_UP)->MoveWindow(bx, srcBtnY, smallBtnW, smallBtnH);
    bx += smallBtnW + srcBtnGap;
    GetDlgItem(IDC_BTN_SOURCE_DOWN)->MoveWindow(bx, srcBtnY, smallBtnW, smallBtnH);

    // Resize source list columns to fill width
    if (m_listSources.GetSafeHwnd())
    {
        int typeColW = 80;
        m_listSources.SetColumnWidth(0, listW - typeColW - 4);
        m_listSources.SetColumnWidth(1, typeColW);
    }

    Invalidate();
}

HBRUSH CLiveClientDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    if (pWnd->GetDlgCtrlID() == IDC_STATIC_STATUS)
    {
        pDC->SetTextColor(m_statusColor);
    }

    return hbr;
}

// --- Scene Management ---

void CLiveClientDlg::OnBtnAddScene()
{
    static int sceneCount = 1;
    CString name;
    name.Format(_T("Scene %d"), (int)m_scenes.size() + 1);

    // Simple input - could be replaced with a dialog
    CObsScene* scene = new CObsScene();
    std::string utf8Name = StringUtil::WideToUtf8(name);
    if (scene->Create(utf8Name.c_str()))
    {
        m_scenes.push_back(scene);
        m_listScenes.AddString(name);
        m_listScenes.SetCurSel((int)m_scenes.size() - 1);
        scene->SetAsCurrent();
        RefreshSourceList();
    }
    else
    {
        delete scene;
    }
}

void CLiveClientDlg::OnBtnRemoveScene()
{
    int sel = m_listScenes.GetCurSel();
    if (sel == LB_ERR || m_scenes.size() <= 1)
    {
        if (m_scenes.size() <= 1)
            AfxMessageBox(_T("Cannot remove the last scene."), MB_ICONWARNING);
        return;
    }

    delete m_scenes[sel];
    m_scenes.erase(m_scenes.begin() + sel);
    m_listScenes.DeleteString(sel);

    int newSel = (sel >= (int)m_scenes.size()) ? (int)m_scenes.size() - 1 : sel;
    m_listScenes.SetCurSel(newSel);
    if (newSel >= 0)
        m_scenes[newSel]->SetAsCurrent();

    RefreshSourceList();
}

void CLiveClientDlg::OnSceneSelChange()
{
    int sel = m_listScenes.GetCurSel();
    if (sel == LB_ERR || sel >= (int)m_scenes.size())
        return;

    m_scenes[sel]->SetAsCurrent();
    RefreshSourceList();
    RefreshMixer();
}

CObsScene* CLiveClientDlg::GetCurrentScene()
{
    int sel = m_listScenes.GetCurSel();
    if (sel == LB_ERR || sel >= (int)m_scenes.size())
        return nullptr;
    return m_scenes[sel];
}

int CLiveClientDlg::GetCurrentSceneIndex()
{
    return m_listScenes.GetCurSel();
}

// --- Source Management ---

void CLiveClientDlg::OnBtnAddSource()
{
    CObsScene* scene = GetCurrentScene();
    if (!scene)
        return;

    CAddSourceDlg dlg(this);
    if (dlg.DoModal() != IDOK)
        return;

    std::string name = StringUtil::WideToUtf8(dlg.m_sourceName);
    CObsSource* source = nullptr;

    switch (dlg.m_selectedType)
    {
    case SourceType::Camera:
        source = CObsSource::CreateCamera(name.c_str());
        break;
    case SourceType::DisplayCapture:
        source = CObsSource::CreateDisplayCapture(name.c_str());
        break;
    case SourceType::WindowCapture:
        source = CObsSource::CreateWindowCapture(name.c_str());
        break;
    case SourceType::Image:
        source = CObsSource::CreateImage(name.c_str());
        break;
    case SourceType::Text:
        source = CObsSource::CreateText(name.c_str(), "Sample Text");
        break;
    }

    if (!source)
    {
        AfxMessageBox(_T("Failed to create source."), MB_ICONERROR);
        return;
    }

    // Add to current scene
    obs_sceneitem_t* item = scene->AddSource(source->GetSource());
    if (!item)
    {
        delete source;
        AfxMessageBox(_T("Failed to add source to scene."), MB_ICONERROR);
        return;
    }

    m_sources.push_back(source);
    RefreshSourceList();
    RefreshMixer();
}

void CLiveClientDlg::OnBtnRemoveSource()
{
    CObsScene* scene = GetCurrentScene();
    if (!scene)
        return;

    int sel = m_listSources.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0)
        return;

    auto items = scene->GetItems();
    if (sel >= (int)items.size())
        return;

    obs_sceneitem_t* item = items[sel].item;
    obs_source_t* source = items[sel].source;

    // Remove from scene
    scene->RemoveItem(item);

    // Remove from our source list
    for (auto it = m_sources.begin(); it != m_sources.end(); ++it)
    {
        if ((*it)->GetSource() == source)
        {
            delete *it;
            m_sources.erase(it);
            break;
        }
    }

    RefreshSourceList();
    RefreshMixer();
}

void CLiveClientDlg::OnBtnSourceProps()
{
    CObsScene* scene = GetCurrentScene();
    if (!scene)
        return;

    int sel = m_listSources.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0)
        return;

    auto items = scene->GetItems();
    if (sel >= (int)items.size())
        return;

    obs_source_t* source = items[sel].source;
    CSourcePropDlg dlg(source, this);
    dlg.DoModal();
}

void CLiveClientDlg::OnBtnSourceUp()
{
    CObsScene* scene = GetCurrentScene();
    if (!scene)
        return;

    int sel = m_listSources.GetNextItem(-1, LVNI_SELECTED);
    if (sel <= 0)
        return;

    auto items = scene->GetItems();
    if (sel < (int)items.size())
    {
        scene->MoveItemUp(items[sel].item);
        RefreshSourceList();
        m_listSources.SetItemState(sel - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void CLiveClientDlg::OnBtnSourceDown()
{
    CObsScene* scene = GetCurrentScene();
    if (!scene)
        return;

    int sel = m_listSources.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0)
        return;

    auto items = scene->GetItems();
    if (sel < (int)items.size() - 1)
    {
        scene->MoveItemDown(items[sel].item);
        RefreshSourceList();
        m_listSources.SetItemState(sel + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void CLiveClientDlg::RefreshSourceList()
{
    m_listSources.DeleteAllItems();

    CObsScene* scene = GetCurrentScene();
    if (!scene)
        return;

    auto items = scene->GetItems();
    for (int i = 0; i < (int)items.size(); i++)
    {
        CString name = StringUtil::Utf8ToWide(items[i].name);
        int idx = m_listSources.InsertItem(i, name);

        // Find source type
        CString typeName = _T("Unknown");
        for (auto* src : m_sources)
        {
            if (src->GetSource() == items[i].source)
            {
                typeName = StringUtil::Utf8ToWide(CObsSource::GetTypeDisplayName(src->GetType()));
                break;
            }
        }
        m_listSources.SetItemText(idx, 1, typeName);
        m_listSources.SetCheck(idx, items[i].visible);
    }
}

// --- Audio Mixer ---

void CLiveClientDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (pScrollBar && pScrollBar->GetSafeHwnd() == m_sliderMaster.GetSafeHwnd())
    {
        int pos = m_sliderMaster.GetPos();
        float vol = (float)pos / 100.0f;

        // Set volume for all audio sources in the current scene
        CObsScene* scene = GetCurrentScene();
        if (scene)
        {
            auto items = scene->GetItems();
            for (auto& item : items)
            {
                obs_source_set_volume(item.source, vol);
            }
        }
    }

    // Beauty sliders
    if (pScrollBar &&
        (pScrollBar->GetSafeHwnd() == m_sliderSmooth.GetSafeHwnd() ||
         pScrollBar->GetSafeHwnd() == m_sliderWhite.GetSafeHwnd()))
    {
        UpdateBeautyFilter();
    }

    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CLiveClientDlg::OnCheckMuteMaster()
{
    BOOL muted = m_checkMuteMaster.GetCheck() == BST_CHECKED;

    CObsScene* scene = GetCurrentScene();
    if (scene)
    {
        auto items = scene->GetItems();
        for (auto& item : items)
        {
            obs_source_set_muted(item.source, muted ? true : false);
        }
    }
}

void CLiveClientDlg::RefreshMixer()
{
    // For now just ensure master slider reflects overall state
    // A more advanced mixer would show per-source sliders
}

// --- Streaming / Recording ---

void CLiveClientDlg::OnBtnStartStream()
{
    if (m_bStreaming)
    {
        m_output.StopStreaming();
        return;
    }

    CObsSettings& settings = CObsSettings::Instance();
    if (settings.m_server.empty() || settings.m_key.empty())
    {
        AfxMessageBox(_T("Please configure the stream server and key in Settings first."),
                       MB_ICONWARNING);
        return;
    }

    // Update service in case settings changed
    m_service.Update(settings.m_server.c_str(), settings.m_key.c_str());
    m_output.SetupStreamOutput(&m_encoder, &m_service);

    if (!m_output.StartStreaming())
    {
        CString err = StringUtil::Utf8ToWide(m_output.GetLastError());
        AfxMessageBox(_T("Failed to start streaming:\n") + err, MB_ICONERROR);
    }
}

void CLiveClientDlg::OnBtnStartRecord()
{
    if (m_bRecording)
    {
        m_output.StopRecording();
        return;
    }

    CObsSettings& settings = CObsSettings::Instance();
    if (settings.m_recordPath.empty())
    {
        AfxMessageBox(_T("Please configure the recording path in Settings first."),
                       MB_ICONWARNING);
        return;
    }

    // Generate recording file path
    SYSTEMTIME st;
    GetLocalTime(&st);
    char filePath[MAX_PATH];
    sprintf_s(filePath, "%s\\%04d-%02d-%02d_%02d-%02d-%02d.%s",
              settings.m_recordPath.c_str(),
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond,
              settings.m_recordFormat.c_str());

    m_output.SetupRecordOutput(&m_encoder, filePath, settings.m_recordFormat.c_str());

    if (!m_output.StartRecording())
    {
        CString err = StringUtil::Utf8ToWide(m_output.GetLastError());
        AfxMessageBox(_T("Failed to start recording:\n") + err, MB_ICONERROR);
    }
}

void CLiveClientDlg::OnBtnSettings()
{
    CSettingsDlg dlg(this);
    if (dlg.DoModal() == IDOK)
    {
        CObsSettings& settings = CObsSettings::Instance();

        // Apply encoder changes if not streaming
        if (!m_bStreaming)
        {
            m_encoder.Release();
            m_encoder.CreateVideoEncoder(settings.m_videoEncoder.c_str(),
                                          settings.m_videoBitrate,
                                          settings.m_keyintSec,
                                          settings.m_rateControl.c_str());
            m_encoder.CreateAudioEncoder(settings.m_audioBitrate);

            m_service.Update(settings.m_server.c_str(), settings.m_key.c_str());
            m_output.SetupStreamOutput(&m_encoder, &m_service);
        }

        // Apply video reset if resolution changed
        CObsApp::Instance().ResetVideo(settings.m_baseWidth, settings.m_baseHeight,
                                        settings.m_outputWidth, settings.m_outputHeight,
                                        settings.m_fpsNum, settings.m_fpsDen);
    }
}

// --- OBS Signal Handlers ---

LRESULT CLiveClientDlg::OnObsStreamStarted(WPARAM, LPARAM)
{
    m_bStreaming = true;
    m_dwStreamStartTime = GetTickCount();
    m_lastTotalBytes = 0;

    GetDlgItem(IDC_BTN_START_STREAM)->SetWindowText(_T("Stop Streaming"));

    m_statusColor = RGB(0, 180, 0); // Green
    GetDlgItem(IDC_STATIC_STATUS)->Invalidate();

    return 0;
}

LRESULT CLiveClientDlg::OnObsStreamStopped(WPARAM, LPARAM)
{
    m_bStreaming = false;
    GetDlgItem(IDC_BTN_START_STREAM)->SetWindowText(_T("Start Streaming"));

    m_statusColor = RGB(128, 128, 128); // Gray
    GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(_T("Offline"));
    GetDlgItem(IDC_STATIC_STATUS)->Invalidate();

    return 0;
}

LRESULT CLiveClientDlg::OnObsStreamError(WPARAM wParam, LPARAM)
{
    m_bStreaming = false;
    GetDlgItem(IDC_BTN_START_STREAM)->SetWindowText(_T("Start Streaming"));

    m_statusColor = RGB(220, 0, 0); // Red
    GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(_T("Stream Error"));
    GetDlgItem(IDC_STATIC_STATUS)->Invalidate();

    int code = (int)wParam;
    CString reason;
    switch (code)
    {
    case OBS_OUTPUT_BAD_PATH:        // -1
        reason = _T("Invalid stream path or URL format.\nCheck that the server address starts with rtmp://");
        break;
    case OBS_OUTPUT_CONNECT_FAILED:  // -2
        reason = _T("Could not connect to the RTMP server.\n\nPlease check:\n")
                 _T("1. The server address is correct (e.g. rtmp://192.168.1.100/live/)\n")
                 _T("2. The RTMP server is running and accepting connections\n")
                 _T("3. The port (default 1935) is not blocked by a firewall\n")
                 _T("4. Network connectivity to the server");
        break;
    case OBS_OUTPUT_INVALID_STREAM:  // -3
        reason = _T("Invalid stream.\nThe server rejected the stream key or the stream path is incorrect.");
        break;
    case OBS_OUTPUT_ERROR:           // -4
        reason = _T("An unexpected streaming error occurred.");
        break;
    case OBS_OUTPUT_DISCONNECTED:    // -5
        reason = _T("Disconnected from the RTMP server.\nThe connection was lost during streaming.");
        break;
    case OBS_OUTPUT_UNSUPPORTED:     // -6
        reason = _T("Unsupported output format or encoder configuration.");
        break;
    case OBS_OUTPUT_NO_SPACE:        // -7
        reason = _T("No disk space available.");
        break;
    default:
        reason.Format(_T("Unknown error (code: %d)"), code);
        break;
    }

    // Also get OBS's own error message if available
    CString obsErr = StringUtil::Utf8ToWide(m_output.GetLastError());

    CString msg;
    msg.Format(_T("Streaming failed:\n\n%s"), (LPCTSTR)reason);
    if (!obsErr.IsEmpty())
        msg += _T("\n\nOBS detail: ") + obsErr;

    AfxMessageBox(msg, MB_ICONERROR);

    return 0;
}

LRESULT CLiveClientDlg::OnObsRecordStarted(WPARAM, LPARAM)
{
    m_bRecording = true;
    m_dwRecordStartTime = GetTickCount();
    GetDlgItem(IDC_BTN_START_RECORD)->SetWindowText(_T("Stop Recording"));
    return 0;
}

LRESULT CLiveClientDlg::OnObsRecordStopped(WPARAM, LPARAM)
{
    m_bRecording = false;
    GetDlgItem(IDC_BTN_START_RECORD)->SetWindowText(_T("Start Recording"));
    return 0;
}

// --- Status Timer ---

void CLiveClientDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == ID_TIMER_STATUS)
        UpdateStatusBar();

    CDialogEx::OnTimer(nIDEvent);
}

void CLiveClientDlg::UpdateStatusBar()
{
    if (!m_bStreaming && !m_bRecording)
    {
        GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(_T("Offline"));
        GetDlgItem(IDC_STATIC_STREAM_TIME)->SetWindowText(_T("00:00:00"));
        return;
    }

    CString status;

    if (m_bStreaming)
    {
        // Calculate bitrate
        uint64_t totalBytes = m_output.GetTotalBytes();
        uint64_t bytesDelta = totalBytes - m_lastTotalBytes;
        m_lastTotalBytes = totalBytes;
        int kbps = (int)(bytesDelta * 8 / 1000); // bits per second / 1000

        // Get FPS and dropped frames
        double fps = obs_get_active_fps();
        int dropped = m_output.GetDroppedFrames();
        int total = m_output.GetTotalFrames();

        status.Format(_T("LIVE | %d kbps | %.1f fps | %d dropped"), kbps, fps, dropped);

        if (m_bRecording)
            status += _T(" | REC");
    }
    else if (m_bRecording)
    {
        status = _T("REC");
    }

    GetDlgItem(IDC_STATIC_STATUS)->SetWindowText(status);

    // Update stream time
    DWORD startTime = m_bStreaming ? m_dwStreamStartTime : m_dwRecordStartTime;
    DWORD elapsed = (GetTickCount() - startTime) / 1000;
    int hours = elapsed / 3600;
    int mins  = (elapsed % 3600) / 60;
    int secs  = elapsed % 60;

    CString timeStr;
    timeStr.Format(_T("%02d:%02d:%02d"), hours, mins, secs);
    GetDlgItem(IDC_STATIC_STREAM_TIME)->SetWindowText(timeStr);
}

// --- Beauty Filter ---

void CLiveClientDlg::OnCheckBeauty()
{
    UpdateBeautyFilter();
}

obs_source_t* CLiveClientDlg::FindBeautyFilter()
{
    // Find a camera source in current scene and get its beauty filter
    CObsScene* scene = GetCurrentScene();
    if (!scene)
        return nullptr;

    auto items = scene->GetItems();
    for (auto& item : items)
    {
        // Check if this source has a beauty_filter attached
        obs_source_t* filter = obs_source_get_filter_by_name(item.source, "Beauty");
        if (filter)
        {
            // obs_source_get_filter_by_name increments refcount, caller must release
            return filter;
        }
    }

    // Also check m_sources for camera types
    for (auto* src : m_sources)
    {
        if (src->GetType() == SourceType::Camera)
        {
            obs_source_t* filter = obs_source_get_filter_by_name(
                src->GetSource(), "Beauty");
            if (filter)
                return filter;
        }
    }

    return nullptr;
}

void CLiveClientDlg::UpdateBeautyFilter()
{
    obs_source_t* filter = FindBeautyFilter();
    if (!filter)
        return;

    bool enabled = (m_checkBeauty.GetCheck() == BST_CHECKED);
    int smooth = m_sliderSmooth.GetPos();
    int white = m_sliderWhite.GetPos();

    obs_data_t* settings = obs_data_create();
    obs_data_set_bool(settings, "enabled", enabled);
    obs_data_set_int(settings, "smooth_level", smooth);
    obs_data_set_int(settings, "white_level", white);
    obs_source_update(filter, settings);
    obs_data_release(settings);
    obs_source_release(filter);
}
