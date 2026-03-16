#pragma once

#include "../resource.h"
#include "../obs/ObsPreview.h"
#include "../obs/ObsScene.h"
#include "../obs/ObsSource.h"
#include "../obs/ObsOutput.h"
#include "../obs/ObsEncoder.h"
#include "../obs/ObsService.h"

class CLiveClientDlg : public CDialogEx
{
public:
    CLiveClientDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_LIVECLIENT_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    DECLARE_MESSAGE_MAP()

    // Message handlers
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnClose();
    afx_msg void OnDestroy();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    // Scene controls
    afx_msg void OnBtnAddScene();
    afx_msg void OnBtnRemoveScene();
    afx_msg void OnSceneSelChange();

    // Source controls
    afx_msg void OnBtnAddSource();
    afx_msg void OnBtnRemoveSource();
    afx_msg void OnBtnSourceProps();
    afx_msg void OnBtnSourceUp();
    afx_msg void OnBtnSourceDown();

    // Audio mixer
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnCheckMuteMaster();

    // Control buttons
    afx_msg void OnBtnStartStream();
    afx_msg void OnBtnStartRecord();
    afx_msg void OnBtnSettings();

    // OBS signal messages
    afx_msg LRESULT OnObsStreamStarted(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnObsStreamStopped(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnObsStreamError(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnObsRecordStarted(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnObsRecordStopped(WPARAM wParam, LPARAM lParam);

private:
    void LayoutControls(int cx, int cy);
    void RefreshSourceList();
    void RefreshMixer();
    void UpdateStatusBar();
    void SetupObs();
    void ShutdownObs();

    CObsScene* GetCurrentScene();
    int GetCurrentSceneIndex();

    // OBS components
    CObsPreview  m_preview;
    CObsOutput   m_output;
    CObsEncoder  m_encoder;
    CObsService  m_service;

    std::vector<CObsScene*>  m_scenes;
    std::vector<CObsSource*> m_sources; // All sources across scenes

    // Controls
    CStatic    m_staticPreview;
    CListBox   m_listScenes;
    CListCtrl  m_listSources;
    CSliderCtrl m_sliderMaster;
    CButton    m_checkMuteMaster;

    // Stream state
    bool       m_bStreaming;
    bool       m_bRecording;
    DWORD      m_dwStreamStartTime;
    DWORD      m_dwRecordStartTime;
    uint64_t   m_lastTotalBytes;

    // Status text color
    COLORREF   m_statusColor;
    CBrush     m_statusBrush;

    HICON      m_hIcon;
};
