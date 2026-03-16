#pragma once

#include "../resource.h"
#include "../obs/ObsSource.h"

class CAddSourceDlg : public CDialogEx
{
public:
    CAddSourceDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_ADDSOURCE_DIALOG };

    SourceType m_selectedType;
    CString    m_sourceName;

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;

    DECLARE_MESSAGE_MAP()

    afx_msg void OnCbnSelchangeSourceType();

private:
    CComboBox m_comboType;
    CEdit     m_editName;
};
