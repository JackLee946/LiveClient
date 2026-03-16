#pragma once

#include <string>
#include <vector>
#include <obs.h>

class CObsScene
{
public:
    CObsScene();
    ~CObsScene();

    bool Create(const char* name);
    void Release();

    // Set this scene as the active output
    void SetAsCurrent();

    // Source management within this scene
    obs_sceneitem_t* AddSource(obs_source_t* source);
    void RemoveItem(obs_sceneitem_t* item);

    // Fit a scene item to the canvas (scale + center)
    void FitItemToCanvas(obs_sceneitem_t* item);

    // Scene item ordering
    void MoveItemUp(obs_sceneitem_t* item);
    void MoveItemDown(obs_sceneitem_t* item);

    // Scene item visibility
    void SetItemVisible(obs_sceneitem_t* item, bool visible);

    // Getters
    obs_scene_t*  GetScene() const { return m_scene; }
    obs_source_t* GetSceneSource() const;
    const std::string& GetName() const { return m_name; }

    // Enumerate scene items
    struct SceneItemInfo
    {
        obs_sceneitem_t* item;
        obs_source_t*    source;
        std::string      name;
        bool             visible;
    };
    std::vector<SceneItemInfo> GetItems() const;

private:
    obs_scene_t* m_scene;
    std::string  m_name;
};
