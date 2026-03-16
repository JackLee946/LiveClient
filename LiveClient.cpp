#include "stdafx.h"
#include "LiveClient.h"
#include "dialogs/LiveClientDlg.h"
#include "obs/ObsApp.h"
#include "obs/ObsSettings.h"
#include "utils/StringUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CLiveClientApp theApp;

BEGIN_MESSAGE_MAP(CLiveClientApp, CWinApp)
END_MESSAGE_MAP()

CLiveClientApp::CLiveClientApp()
{
}

BOOL CLiveClientApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    SetRegistryKey(_T("LiveClient"));

    // Load settings
    CObsSettings& settings = CObsSettings::Instance();
    settings.Load();

    // Initialize OBS
    CObsApp& obsApp = CObsApp::Instance();
    if (!obsApp.Initialize(settings))
    {
        CString msg = _T("Failed to initialize OBS.\n\n");
        msg += StringUtil::Utf8ToWide(obsApp.GetLastError());
        AfxMessageBox(msg, MB_ICONERROR);
        return FALSE;
    }

    // Show main dialog
    CLiveClientDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}

int CLiveClientApp::ExitInstance()
{
    CObsApp::Instance().Shutdown();
    return CWinApp::ExitInstance();
}
