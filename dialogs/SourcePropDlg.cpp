#include "stdafx.h"
#include "SourcePropDlg.h"
#include "../utils/StringUtil.h"

CSourcePropDlg::CSourcePropDlg(obs_source_t* source, CWnd* pParent)
    : CDialogEx(IDD_SOURCEPROP_DIALOG, pParent)
    , m_source(source)
    , m_props(nullptr)
    , m_nextCtrlId(2000)
    , m_yOffset(14)
{
}

CSourcePropDlg::~CSourcePropDlg()
{
    for (auto& pc : m_propControls)
    {
        delete pc.control;
        delete pc.label;
        delete pc.browseBtn;
    }
    m_propControls.clear();

    if (m_props)
        obs_properties_destroy(m_props);
}

void CSourcePropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSourcePropDlg, CDialogEx)
    ON_CONTROL_RANGE(BN_CLICKED, 3000, 3099, &CSourcePropDlg::OnBrowseButton)
END_MESSAGE_MAP()

BOOL CSourcePropDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Set title to source name
    if (m_source)
    {
        CString title;
        title.Format(_T("Properties - %s"), StringUtil::Utf8ToWide(obs_source_get_name(m_source)).GetString());
        SetWindowText(title);
    }

    BuildControls();
    return TRUE;
}

void CSourcePropDlg::BuildControls()
{
    if (!m_source)
        return;

    m_props = obs_source_properties(m_source);
    if (!m_props)
        return;

    obs_data_t* settings = obs_source_get_settings(m_source);
    CWnd* propArea = GetDlgItem(IDC_STATIC_PROP_AREA);
    if (!propArea)
        return;

    CRect rcArea;
    propArea->GetClientRect(&rcArea);
    propArea->MapWindowPoints(this, &rcArea);

    int xLabel = rcArea.left + 8;
    int xCtrl  = rcArea.left + 120;
    int ctrlW  = rcArea.Width() - 136;
    int ctrlH  = 20;
    int labelH = 14;
    int spacing = 28;
    int y = rcArea.top + 10;

    obs_property_t* prop = obs_properties_first(m_props);
    while (prop)
    {
        if (!obs_property_visible(prop))
        {
            obs_property_next(&prop);
            continue;
        }

        const char* propName = obs_property_name(prop);
        const char* propDesc = obs_property_description(prop);
        obs_property_type propType = obs_property_get_type(prop);

        CString desc = StringUtil::Utf8ToWide(propDesc ? propDesc : propName);

        PropControl pc = {};
        pc.prop = prop;
        pc.type = propType;

        // Create label
        pc.label = new CStatic();
        pc.label->Create(desc, WS_CHILD | WS_VISIBLE | SS_LEFT,
                         CRect(xLabel, y + 2, xCtrl - 4, y + labelH + 2),
                         this, m_nextCtrlId++);
        pc.label->SetFont(GetFont());

        // Create control based on property type
        switch (propType)
        {
        case OBS_PROPERTY_BOOL:
        {
            CButton* btn = new CButton();
            btn->Create(_T(""), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        CRect(xCtrl, y, xCtrl + ctrlW, y + ctrlH),
                        this, m_nextCtrlId++);
            btn->SetFont(GetFont());

            bool val = obs_data_get_bool(settings, propName);
            btn->SetCheck(val ? BST_CHECKED : BST_UNCHECKED);

            pc.control = btn;
            break;
        }

        case OBS_PROPERTY_INT:
        {
            CEdit* edit = new CEdit();
            edit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                         CRect(xCtrl, y, xCtrl + 80, y + ctrlH),
                         this, m_nextCtrlId++);
            edit->SetFont(GetFont());

            int val = (int)obs_data_get_int(settings, propName);
            CString str;
            str.Format(_T("%d"), val);
            edit->SetWindowText(str);

            pc.control = edit;
            break;
        }

        case OBS_PROPERTY_FLOAT:
        {
            CEdit* edit = new CEdit();
            edit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER,
                         CRect(xCtrl, y, xCtrl + 80, y + ctrlH),
                         this, m_nextCtrlId++);
            edit->SetFont(GetFont());

            double val = obs_data_get_double(settings, propName);
            CString str;
            str.Format(_T("%.2f"), val);
            edit->SetWindowText(str);

            pc.control = edit;
            break;
        }

        case OBS_PROPERTY_TEXT:
        {
            CEdit* edit = new CEdit();
            DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
            edit->Create(style,
                         CRect(xCtrl, y, xCtrl + ctrlW, y + ctrlH),
                         this, m_nextCtrlId++);
            edit->SetFont(GetFont());

            const char* val = obs_data_get_string(settings, propName);
            edit->SetWindowText(StringUtil::Utf8ToWide(val ? val : ""));

            pc.control = edit;
            break;
        }

        case OBS_PROPERTY_PATH:
        {
            int btnW = 30;
            int editW = ctrlW - btnW - 4;

            CEdit* edit = new CEdit();
            DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
            edit->Create(style,
                         CRect(xCtrl, y, xCtrl + editW, y + ctrlH),
                         this, m_nextCtrlId++);
            edit->SetFont(GetFont());

            const char* val = obs_data_get_string(settings, propName);
            edit->SetWindowText(StringUtil::Utf8ToWide(val ? val : ""));

            // Browse button (ID range 3000-3099 for ON_CONTROL_RANGE)
            int browseBtnId = 3000 + (int)m_propControls.size();
            CButton* btn = new CButton();
            btn->Create(_T("..."), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        CRect(xCtrl + editW + 4, y, xCtrl + ctrlW, y + ctrlH),
                        this, browseBtnId);
            btn->SetFont(GetFont());

            pc.control = edit;
            pc.browseBtn = btn;
            break;
        }

        case OBS_PROPERTY_LIST:
        {
            CComboBox* combo = new CComboBox();
            combo->Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                          CRect(xCtrl, y, xCtrl + ctrlW, y + ctrlH + 120),
                          this, m_nextCtrlId++);
            combo->SetFont(GetFont());

            obs_combo_format format = obs_property_list_format(prop);
            size_t count = obs_property_list_item_count(prop);

            const char* currentStr = obs_data_get_string(settings, propName);
            long long currentInt = obs_data_get_int(settings, propName);

            for (size_t i = 0; i < count; i++)
            {
                const char* itemName = obs_property_list_item_name(prop, i);
                CString name = StringUtil::Utf8ToWide(itemName ? itemName : "");
                int idx = combo->AddString(name);

                if (format == OBS_COMBO_FORMAT_STRING)
                {
                    const char* itemVal = obs_property_list_item_string(prop, i);
                    // Store string index for matching
                    if (currentStr && itemVal && strcmp(currentStr, itemVal) == 0)
                        combo->SetCurSel(idx);
                }
                else if (format == OBS_COMBO_FORMAT_INT)
                {
                    long long itemVal = obs_property_list_item_int(prop, i);
                    combo->SetItemData(idx, (DWORD_PTR)itemVal);
                    if (itemVal == currentInt)
                        combo->SetCurSel(idx);
                }
            }

            if (combo->GetCurSel() == CB_ERR && combo->GetCount() > 0)
                combo->SetCurSel(0);

            pc.control = combo;
            break;
        }

        default:
        {
            // For unsupported types, create a static text
            CStatic* st = new CStatic();
            st->Create(_T("(not editable)"), WS_CHILD | WS_VISIBLE,
                       CRect(xCtrl, y, xCtrl + ctrlW, y + ctrlH),
                       this, m_nextCtrlId++);
            st->SetFont(GetFont());
            pc.control = st;
            break;
        }
        }

        m_propControls.push_back(pc);
        y += spacing;

        obs_property_next(&prop);
    }

    obs_data_release(settings);
}

void CSourcePropDlg::OnOK()
{
    ApplySettings();
    CDialogEx::OnOK();
}

void CSourcePropDlg::ApplySettings()
{
    if (!m_source || !m_props)
        return;

    obs_data_t* settings = obs_data_create();

    for (auto& pc : m_propControls)
    {
        const char* propName = obs_property_name(pc.prop);

        switch (pc.type)
        {
        case OBS_PROPERTY_BOOL:
        {
            CButton* btn = static_cast<CButton*>(pc.control);
            bool val = (btn->GetCheck() == BST_CHECKED);
            obs_data_set_bool(settings, propName, val);
            break;
        }

        case OBS_PROPERTY_INT:
        {
            CEdit* edit = static_cast<CEdit*>(pc.control);
            CString str;
            edit->GetWindowText(str);
            obs_data_set_int(settings, propName, _ttoi(str));
            break;
        }

        case OBS_PROPERTY_FLOAT:
        {
            CEdit* edit = static_cast<CEdit*>(pc.control);
            CString str;
            edit->GetWindowText(str);
            obs_data_set_double(settings, propName, _ttof(str));
            break;
        }

        case OBS_PROPERTY_TEXT:
        case OBS_PROPERTY_PATH:
        {
            CEdit* edit = static_cast<CEdit*>(pc.control);
            CString str;
            edit->GetWindowText(str);
            obs_data_set_string(settings, propName, StringUtil::WideToUtf8(str).c_str());
            break;
        }

        case OBS_PROPERTY_LIST:
        {
            CComboBox* combo = static_cast<CComboBox*>(pc.control);
            int sel = combo->GetCurSel();
            if (sel == CB_ERR)
                break;

            obs_combo_format format = obs_property_list_format(pc.prop);
            if (format == OBS_COMBO_FORMAT_STRING)
            {
                // Get the original string value for this index
                const char* val = obs_property_list_item_string(pc.prop, sel);
                if (val)
                    obs_data_set_string(settings, propName, val);
            }
            else if (format == OBS_COMBO_FORMAT_INT)
            {
                long long val = obs_property_list_item_int(pc.prop, sel);
                obs_data_set_int(settings, propName, val);
            }
            break;
        }
        }
    }

    obs_source_update(m_source, settings);
    obs_data_release(settings);
}

void CSourcePropDlg::OnBrowseButton(UINT nID)
{
    int index = nID - 3000;
    if (index < 0 || index >= (int)m_propControls.size())
        return;

    PropControl& pc = m_propControls[index];
    if (pc.type != OBS_PROPERTY_PATH)
        return;

    obs_path_type pathType = obs_property_path_type(pc.prop);
    const char* filter = obs_property_path_filter(pc.prop);
    const char* defaultPath = obs_property_path_default_path(pc.prop);

    // Build filter string in a vector<TCHAR> buffer (supports embedded \0)
    // OBS format: "Images (*.png *.jpg);;All Files (*.*)"
    // MFC format: "Images\0*.png;*.jpg\0All Files\0*.*\0\0"
    std::vector<TCHAR> filterBuf;

    if (filter && filter[0])
    {
        std::string f(filter);
        size_t pos = 0;
        while (pos < f.size())
        {
            size_t end = f.find(";;", pos);
            if (end == std::string::npos) end = f.size();

            std::string group = f.substr(pos, end - pos);

            size_t pOpen = group.find('(');
            size_t pClose = group.rfind(')');
            if (pOpen != std::string::npos && pClose != std::string::npos && pClose > pOpen)
            {
                std::string desc = group.substr(0, pOpen);
                while (!desc.empty() && desc.back() == ' ') desc.pop_back();
                std::string pattern = group.substr(pOpen + 1, pClose - pOpen - 1);
                for (auto& c : pattern) { if (c == ' ') c = ';'; }

                // Append "Description\0pattern\0"
                CString wDesc = StringUtil::Utf8ToWide(desc);
                CString wPat  = StringUtil::Utf8ToWide(pattern);
                for (int i = 0; i < wDesc.GetLength(); i++)
                    filterBuf.push_back(wDesc[i]);
                filterBuf.push_back(_T('\0'));
                for (int i = 0; i < wPat.GetLength(); i++)
                    filterBuf.push_back(wPat[i]);
                filterBuf.push_back(_T('\0'));
            }

            pos = (end < f.size()) ? end + 2 : f.size();
        }
    }

    if (filterBuf.empty())
    {
        // Default: "All Files\0*.*\0"
        const TCHAR* def = _T("All Files");
        while (*def) filterBuf.push_back(*def++);
        filterBuf.push_back(_T('\0'));
        const TCHAR* pat = _T("*.*");
        while (*pat) filterBuf.push_back(*pat++);
        filterBuf.push_back(_T('\0'));
    }

    // Double null terminator
    filterBuf.push_back(_T('\0'));

    if (pathType == OBS_PATH_DIRECTORY)
    {
        CFolderPickerDialog dlg(nullptr, OFN_FILEMUSTEXIST, this);
        if (dlg.DoModal() == IDOK)
        {
            CEdit* edit = static_cast<CEdit*>(pc.control);
            edit->SetWindowText(dlg.GetPathName());
        }
    }
    else
    {
        DWORD flags = (pathType == OBS_PATH_FILE) ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT;
        BOOL openFile = (pathType == OBS_PATH_FILE);

        CFileDialog dlg(openFile, nullptr, nullptr, flags, nullptr, this);
        dlg.m_ofn.lpstrFilter = filterBuf.data();

        if (defaultPath && defaultPath[0])
        {
            CString wDefaultPath = StringUtil::Utf8ToWide(defaultPath);
            dlg.m_ofn.lpstrInitialDir = wDefaultPath;
        }

        if (dlg.DoModal() == IDOK)
        {
            CEdit* edit = static_cast<CEdit*>(pc.control);
            edit->SetWindowText(dlg.GetPathName());
        }
    }
}
