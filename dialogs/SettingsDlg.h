#pragma once

#include "../resource.h"

class CSettingsDlg : public CDialogEx
{
public:
    CSettingsDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_SETTINGS_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;

    DECLARE_MESSAGE_MAP()

    afx_msg void OnTcnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBtnBrowseRecordPath();

private:
    void ShowTabPage(int tab);
    void LoadSettings();
    void SaveSettings();

    CTabCtrl m_tabCtrl;

    // Tab page control IDs for show/hide
    enum TabPage { TAB_STREAM = 0, TAB_OUTPUT, TAB_VIDEO, TAB_AUDIO, TAB_COUNT };

    // Stream tab IDs
    static const UINT s_streamIds[];
    // Output tab IDs
    static const UINT s_outputIds[];
    // Video tab IDs
    static const UINT s_videoIds[];
    // Audio tab IDs
    static const UINT s_audioIds[];

    int m_currentTab;
};
