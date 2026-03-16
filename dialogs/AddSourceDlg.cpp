#include "stdafx.h"
#include "AddSourceDlg.h"
#include "../utils/StringUtil.h"

CAddSourceDlg::CAddSourceDlg(CWnd* pParent)
    : CDialogEx(IDD_ADDSOURCE_DIALOG, pParent)
    , m_selectedType(SourceType::Camera)
{
}

void CAddSourceDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_SOURCE_TYPE, m_comboType);
    DDX_Control(pDX, IDC_EDIT_SOURCE_NAME, m_editName);
}

BEGIN_MESSAGE_MAP(CAddSourceDlg, CDialogEx)
    ON_CBN_SELCHANGE(IDC_COMBO_SOURCE_TYPE, &CAddSourceDlg::OnCbnSelchangeSourceType)
END_MESSAGE_MAP()

BOOL CAddSourceDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Add source types
    struct TypeEntry
    {
        SourceType type;
        LPCTSTR    name;
    };

    TypeEntry types[] = {
        { SourceType::Camera,         _T("Camera") },
        { SourceType::DisplayCapture, _T("Display Capture") },
        { SourceType::WindowCapture,  _T("Window Capture") },
        { SourceType::Image,          _T("Image") },
        { SourceType::Text,           _T("Text") },
    };

    for (const auto& t : types)
    {
        int idx = m_comboType.AddString(t.name);
        m_comboType.SetItemData(idx, (DWORD_PTR)t.type);
    }

    m_comboType.SetCurSel(0);
    OnCbnSelchangeSourceType(); // Set default name

    return TRUE;
}

void CAddSourceDlg::OnCbnSelchangeSourceType()
{
    int sel = m_comboType.GetCurSel();
    if (sel == CB_ERR)
        return;

    CString typeName;
    m_comboType.GetLBText(sel, typeName);
    m_editName.SetWindowText(typeName);
}

void CAddSourceDlg::OnOK()
{
    int sel = m_comboType.GetCurSel();
    if (sel == CB_ERR)
    {
        AfxMessageBox(_T("Please select a source type."), MB_ICONWARNING);
        return;
    }

    m_selectedType = (SourceType)m_comboType.GetItemData(sel);
    m_editName.GetWindowText(m_sourceName);

    if (m_sourceName.IsEmpty())
    {
        AfxMessageBox(_T("Please enter a source name."), MB_ICONWARNING);
        return;
    }

    CDialogEx::OnOK();
}
