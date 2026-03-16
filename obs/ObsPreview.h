#pragma once

#include <cstdint>
#include <obs.h>

class CObsPreview
{
public:
    CObsPreview();
    ~CObsPreview();

    bool Create(HWND hwnd, uint32_t cx, uint32_t cy, uint32_t bgColor = 0x000000);
    void Resize(uint32_t cx, uint32_t cy);
    void Destroy();

    bool IsCreated() const { return m_display != nullptr; }

private:
    static void DrawCallback(void* param, uint32_t cx, uint32_t cy);

    obs_display_t* m_display;
    HWND           m_hwnd;
};
