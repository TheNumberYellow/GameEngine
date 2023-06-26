#pragma once

#include "Modules/CollisionModule.h"
#include "Modules/GraphicsModule.h"
#include "Modules/UIModule.h"

#include <string>

typedef uint64_t Entity_ID;

// TODO(Fraser): Move this to some reader/asset manager file
enum FileReaderState
{
    TEXTURES,
    STATIC_MESHES,
    ENTITIES,
    NONE
};

struct SceneRayCastHit
{
    RayCastHit rayCastHit;
    Model* hitModel;
};

SceneRayCastHit Closer(const SceneRayCastHit& lhs, const SceneRayCastHit& rhs);

class Scene
{
public:
    Scene();
    ~Scene();

    void Init(GraphicsModule& graphics, CollisionModule& collisions);

    Model* AddModel(Model model, std::string name = "");
    Model* GetModel(std::string name);
    void DeleteModel(Model* model);

    void AddCamera(Camera* camera);

    Camera* GetCamera();
    void SetCamera(Camera* camera);

    void Update();

    void Draw(GraphicsModule& graphics, Framebuffer_ID buffer);
    void EditorDraw(GraphicsModule& graphics, Framebuffer_ID buffer);

    void SetDirectionalLight(DirectionalLight light);

    SceneRayCastHit RayCast(Ray ray, CollisionModule& collision, std::vector<Model*> IgnoredModels = std::vector<Model*>());

    Model* MenuListEntities(UIModule& ui, Font& font);

    void Save(std::string FileName);

    void Load(std::string FileName);
private:
    bool IsIgnored(Model* model, std::vector<Model*> ignoredModels);

    std::unordered_map<std::string, Model*> m_Models;
    std::vector<Model*> m_UntrackedModels;
    DirectionalLight m_DirLight;

    std::vector<Camera*> m_Cameras;

    Camera m_ShadowCamera;
    Framebuffer_ID shadowBuffer;

    GraphicsModule* m_Graphics;
    CollisionModule* m_Collisions;

    static bool GetReaderStateFromToken(std::string Token, FileReaderState& OutState);

};
