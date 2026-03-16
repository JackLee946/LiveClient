#pragma once

#include "resource.h"

class CLiveClientApp : public CWinApp
{
public:
    CLiveClientApp();

    virtual BOOL InitInstance() override;
    virtual int  ExitInstance() override;

    DECLARE_MESSAGE_MAP()
};

extern CLiveClientApp theApp;
