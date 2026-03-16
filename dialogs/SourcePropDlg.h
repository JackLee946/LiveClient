#pragma once

#include "../resource.h"
#include <obs.h>
#include <vector>

class CSourcePropDlg : public CDialogEx
{
public:
    CSourcePropDlg(obs_source_t* source, CWnd* pParent = nullptr);
    ~CSourcePropDlg();

    enum { IDD = IDD_SOURCEPROP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;

    DECLARE_MESSAGE_MAP()
    afx_msg void OnBrowseButton(UINT nID);

private:
    void BuildControls();
    void ApplySettings();

    obs_source_t*     m_source;
    obs_properties_t* m_props;

    struct PropControl
    {
        obs_property_t* prop;
        CWnd*           control;
        CStatic*        label;
        CButton*        browseBtn;  // For OBS_PROPERTY_PATH
        int             type; // obs_property_type
    };
    std::vector<PropControl> m_propControls;

    int m_nextCtrlId;
    int m_yOffset;
};
