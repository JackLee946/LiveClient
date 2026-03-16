#include "stdafx.h"
#include "ObsScene.h"

CObsScene::CObsScene()
    : m_scene(nullptr)
{
}

CObsScene::~CObsScene()
{
    Release();
}

bool CObsScene::Create(const char* name)
{
    if (m_scene)
        return false;

    m_scene = obs_scene_create(name);
    if (!m_scene)
        return false;

    m_name = name;
    return true;
}

void CObsScene::Release()
{
    if (m_scene)
    {
        obs_scene_release(m_scene);
        m_scene = nullptr;
    }
    m_name.clear();
}

void CObsScene::SetAsCurrent()
{
    if (!m_scene)
        return;

    obs_source_t* sceneSource = obs_scene_get_source(m_scene);
    if (sceneSource)
        obs_set_output_source(0, sceneSource);
}

obs_source_t* CObsScene::GetSceneSource() const
{
    if (!m_scene)
        return nullptr;
    return obs_scene_get_source(m_scene);
}

obs_sceneitem_t* CObsScene::AddSource(obs_source_t* source)
{
    if (!m_scene || !source)
        return nullptr;

    obs_sceneitem_t* item = obs_scene_add(m_scene, source);
    if (item)
        FitItemToCanvas(item);

    return item;
}

void CObsScene::FitItemToCanvas(obs_sceneitem_t* item)
{
    if (!item)
        return;

    // Get canvas size
    obs_video_info ovi;
    if (!obs_get_video_info(&ovi))
        return;

    uint32_t canvasW = ovi.base_width;
    uint32_t canvasH = ovi.base_height;

    // Use bounds to scale the source to fit canvas while keeping aspect ratio
    struct vec2 bounds;
    bounds.x = (float)canvasW;
    bounds.y = (float)canvasH;

    obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
    obs_sceneitem_set_bounds(item, &bounds);

    // Center the item on canvas
    // Get the source's actual size after bounds are applied
    obs_source_t* source = obs_sceneitem_get_source(item);
    if (!source)
        return;

    uint32_t srcW = obs_source_get_width(source);
    uint32_t srcH = obs_source_get_height(source);

    if (srcW == 0 || srcH == 0)
    {
        // Source not yet ready (e.g. async source), just position at origin
        struct vec2 pos = { 0.0f, 0.0f };
        obs_sceneitem_set_pos(item, &pos);
        return;
    }

    // Calculate the scaled size to find the centering offset
    float scaleX = (float)canvasW / (float)srcW;
    float scaleY = (float)canvasH / (float)srcH;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    float drawW = srcW * scale;
    float drawH = srcH * scale;

    struct vec2 pos;
    pos.x = ((float)canvasW - drawW) / 2.0f;
    pos.y = ((float)canvasH - drawH) / 2.0f;
    obs_sceneitem_set_pos(item, &pos);
}

void CObsScene::RemoveItem(obs_sceneitem_t* item)
{
    if (item)
        obs_sceneitem_remove(item);
}

void CObsScene::MoveItemUp(obs_sceneitem_t* item)
{
    if (item)
        obs_sceneitem_set_order(item, OBS_ORDER_MOVE_UP);
}

void CObsScene::MoveItemDown(obs_sceneitem_t* item)
{
    if (item)
        obs_sceneitem_set_order(item, OBS_ORDER_MOVE_DOWN);
}

void CObsScene::SetItemVisible(obs_sceneitem_t* item, bool visible)
{
    if (item)
        obs_sceneitem_set_visible(item, visible);
}

struct EnumData
{
    std::vector<CObsScene::SceneItemInfo>* items;
};

static bool EnumSceneItems(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
    EnumData* data = static_cast<EnumData*>(param);

    obs_source_t* source = obs_sceneitem_get_source(item);
    if (!source)
        return true;

    CObsScene::SceneItemInfo info;
    info.item    = item;
    info.source  = source;
    info.name    = obs_source_get_name(source);
    info.visible = obs_sceneitem_visible(item);

    data->items->push_back(info);
    return true; // continue enumeration
}

std::vector<CObsScene::SceneItemInfo> CObsScene::GetItems() const
{
    std::vector<SceneItemInfo> items;
    if (!m_scene)
        return items;

    EnumData data;
    data.items = &items;
    obs_scene_enum_items(m_scene, EnumSceneItems, &data);
    return items;
}
