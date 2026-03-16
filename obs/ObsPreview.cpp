#include "stdafx.h"
#include "ObsPreview.h"

CObsPreview::CObsPreview()
    : m_display(nullptr)
    , m_hwnd(nullptr)
{
}

CObsPreview::~CObsPreview()
{
    Destroy();
}

bool CObsPreview::Create(HWND hwnd, uint32_t cx, uint32_t cy, uint32_t bgColor)
{
    if (m_display)
        return true;

    m_hwnd = hwnd;

    gs_init_data initData = {};
    initData.cx           = cx;
    initData.cy           = cy;
    initData.format       = GS_BGRA;
    initData.zsformat     = GS_ZS_NONE;
    initData.window.hwnd  = hwnd;

    m_display = obs_display_create(&initData, bgColor);
    if (!m_display)
        return false;

    obs_display_add_draw_callback(m_display, DrawCallback, this);
    return true;
}

void CObsPreview::Resize(uint32_t cx, uint32_t cy)
{
    if (m_display)
        obs_display_resize(m_display, cx, cy);
}

void CObsPreview::Destroy()
{
    if (m_display)
    {
        obs_display_remove_draw_callback(m_display, DrawCallback, this);
        obs_display_destroy(m_display);
        m_display = nullptr;
    }
    m_hwnd = nullptr;
}

void CObsPreview::DrawCallback(void* param, uint32_t cx, uint32_t cy)
{
    CObsPreview* self = static_cast<CObsPreview*>(param);
    if (!self)
        return;

    // Get the current output video info for aspect ratio calculation
    obs_video_info ovi;
    if (!obs_get_video_info(&ovi))
        return;

    // Calculate aspect-ratio-correct scaling
    uint32_t sourceW = ovi.base_width;
    uint32_t sourceH = ovi.base_height;

    float scaleX = (float)cx / (float)sourceW;
    float scaleY = (float)cy / (float)sourceH;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    int drawW = (int)(sourceW * scale);
    int drawH = (int)(sourceH * scale);
    int offsetX = (cx - drawW) / 2;
    int offsetY = (cy - drawH) / 2;

    // Set the viewport for centered rendering
    gs_viewport_push();
    gs_projection_push();

    gs_ortho(0.0f, (float)sourceW, 0.0f, (float)sourceH, -100.0f, 100.0f);
    gs_set_viewport(offsetX, offsetY, drawW, drawH);

    // Render the main OBS output
    obs_render_main_texture();

    gs_projection_pop();
    gs_viewport_pop();
}
