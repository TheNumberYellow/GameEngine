#include "EditorState.h"

#include "States/GameState.h"

#include <filesystem>
#include <ctime>



void EditorState::OnInitialized()
{
    GraphicsModule* Graphics = GraphicsModule::Get();
    CollisionModule* Collisions = CollisionModule::Get();
    TextModule* Text = TextModule::Get();
    InputModule* Input = InputModule::Get();

    LoadEditorResources();

    // Load user resources
    LoadedModels = LoadModels(*Graphics);
    LoadedMaterials = LoadMaterials(*Graphics);

    // Create editor viewport camera
    ViewportCamera = Camera(Projection::Perspective);
    ViewportCamera.SetScreenSize(GetEditorSceneViewportRect().size);

    // TODO: Hook up viewport camera to editor "player"? (Might want to allow entities to parented together more generically?)

    // Create model camera
    ModelCamera = Camera(Projection::Perspective);
    ModelCamera.SetScreenSize(Vec2f(100.0f, 100.0f));
    ModelCamera.SetPosition(Vec3f(0.0f, -2.5f, 2.5f));
    ModelCamera.Rotate(Quaternion(Vec3f(1.0f, 0.0f, 0.0f), -0.7f));
    
    // Set up empty editor scene
    EditorScene.SetDirectionalLight(DirectionalLight{ Math::normalize(Vec3f(0.5f, 1.0f, -1.0f)), Vec3f(1.0f, 1.0f, 1.0f) });
    //EditorScene.SetCamera(&ViewportCamera);
    //EditorScene.SetCamera(&ModelCamera);

    Cursor = CursorState(this, &EditorScene);

    // Set up framebuffers the editor uses
    Rect ViewportRect = GetEditorSceneViewportRect();
    
    ViewportBuffer = Graphics->CreateGBuffer(ViewportRect.size);
    
    WidgetBuffer = Graphics->CreateFBuffer(ViewportRect.size);

    Vec2i NewCenter = Vec2i(ViewportRect.Center());
    //TODO(fraser): clean up mouse constrain/input code
    Input->SetMouseCenter(NewCenter);

    Graphics->InitializeDebugDraw(ViewportBuffer.FinalOutput);

    Graphics->SetRenderMode(RenderMode::DEFAULT);

    CurrentResourceDirectoryPath = std::filesystem::current_path();

    TestFont = TextModule::Get()->LoadFont("fonts/ARLRDBD.TTF", 30);

    for (int i = 0; i < 1000; i++)
    {
        RandomSizes.push_back(Math::RandomFloat(120.0f, 200.0f));
        Vec3f Colour = Vec3f(Math::RandomFloat(0.0f, 1.0f), Math::RandomFloat(0.0f, 1.0f), Math::RandomFloat(0.0f, 1.0f));
        RandomColours.push_back(Colour);
    }
}

void EditorState::OnUninitialized()
{
}

void EditorState::OnEnter()
{
}

void EditorState::OnExit()
{
}

void EditorState::Update(float DeltaTime)
{
#if 0
    UIModule* UI = UIModule::Get();
    GraphicsModule* Graphics = GraphicsModule::Get();

    //Graphics->ResetFrameBuffer();

    UI->StartFrame("Test", Rect(Vec2f(50.0f, 50.0f), Vec2f(600.0f, 400.0f)), 16.0f, c_LightGoldenRodYellow);
    {
        UI->StartFrame("Inner Test", Vec2f(400.0f, 150.0f), 16.0f, c_VegasGold);
        {
            for (int i = 0; i < 1000; ++i)
            {
                UI->TextButton("", Vec2f(20.0f, 20.0f), 4.0f, RandomColours[i]);
            }
        }
        UI->EndFrame();

        for (int i = 0; i < 1000; ++i)
        {
            UI->TextButton("", Vec2f(80.0f, 80.0f), 4.0f, RandomColours[i]);
        }
    }
    UI->EndFrame();

    UI->StartFrame("Test 2", Rect(Vec2f(50.0f, 500.0f), Vec2f(600.0f, 400.0f)), 16.0f, c_DarkOrange);
    {
        for (int i = 0; i < 1000; ++i)
        {
            UI->TextButton("", Vec2f(80.0f, 80.0f), 4.0f, RandomColours[i]);
        }
    }
    UI->EndFrame();

#else    
    UpdateEditor(DeltaTime);
#endif
}

void EditorState::OnResize()
{
    GraphicsModule* graphics = GraphicsModule::Get();
    InputModule* input = InputModule::Get();

    Rect ViewportRect = GetEditorSceneViewportRect();

    ViewportCamera.SetScreenSize(ViewportRect.size);
    graphics->ResizeGBuffer(ViewportBuffer, ViewportRect.size);
    input->SetMouseCenter(ViewportRect.Center());
}

void EditorState::UpdateEditor(float DeltaTime)
{
    GraphicsModule* Graphics = GraphicsModule::Get();
    CollisionModule* Collisions = CollisionModule::Get();
    TextModule* Text = TextModule::Get();
    UIModule* UI = UIModule::Get();
    InputModule* Input = InputModule::Get();

    if (CursorLocked)
    {
        MoveCamera(&ViewportCamera, 0.001f, DeltaTime);
    }

    if (Input->IsKeyDown(Key::Q))
    {

        EditorScene.SetDirectionalLight(DirectionalLight{ ViewportCamera.GetDirection(), Vec3f(1.0f, 1.0f, 1.0f) });

    }

    if (Engine::IsWindowFocused())
    {
        // Update tools
        // TODO(Fraser): This is heavily reliant on order since Update() calls UI functions - another reason to make the UI module use deferred render commands
        Cursor.Update(DeltaTime);
        if (Input->GetKeyState(Key::Alt).justPressed)
        {
            if (CursorLocked)
            {
                Engine::UnlockCursor();
                Engine::ShowCursor();
                CursorLocked = false;
            }
            else {
                Engine::LockCursor();
                Engine::HideCursor();
                CursorLocked = true;
            }
        }
    }
    else
    {
        Cursor.ResetAllState();
    }

    if (Input->IsKeyDown(Key::E))
    {
        EditorScene.GetCamera()->SetPosition(ViewportCamera.GetPosition());
        EditorScene.GetCamera()->SetDirection(ViewportCamera.GetDirection());

        //EditorScene.GetCamera()->SetCamMatrix(ViewportCamera.GetInvCamMatrix());
    }

    // Keyboard hotkeys for switching tools
    if (Input->GetKeyState(Key::One).justPressed)
    {
        // If already in select tool, cycle sub-tool
        if (Cursor.GetToolMode() == ToolMode::Select)
        {
            Cursor.CycleSelectMode();
        }
        else
        {
            Cursor.SetToolMode(ToolMode::Select);
        }
    }
    if (Input->GetKeyState(Key::Two).justPressed)
    {
        // If already in transform tool, cycle sub-tool
        if (Cursor.GetToolMode() == ToolMode::Transform)
        {
            Cursor.CycleTransformMode();
        }
        else
        {
            Cursor.SetToolMode(ToolMode::Transform);
        }
    }
    if (Input->GetKeyState(Key::Three).justPressed)
    {
        // If already in geometry tool, cycle sub-tool
        if (Cursor.GetToolMode() == ToolMode::Geometry)
        {
            Cursor.CycleGeometryMode();
        }
        else
        {       
            Cursor.SetToolMode(ToolMode::Geometry);
        }
    }
    if (Input->GetKeyState(Key::Four).justPressed)
    {
        Cursor.SetToolMode(ToolMode::Sculpt);
    }

    if (Input->GetKeyState(Key::Five).justPressed)
    {
        Cursor.SetToolMode(ToolMode::Brush);
    }

    Vec2i ViewportSize = Engine::GetClientAreaSize();

    //Graphics->ResetFrameBuffer();

    UI->StartFrame("EditorFrame", Rect(Vec2f(0.0f, 0.0f), ViewportSize), 0.0f, c_FrameDark);
    {
        UI->StartTab("Level Editor");
        {
            DrawLevelEditor(Graphics, UI, DeltaTime);
        }
        UI->EndTab();

        UI->StartTab("Material Editor");
        {

        }
        UI->EndTab();       
    }
    UI->EndFrame();

}

void EditorState::UpdateGame(float DeltaTime)
{
}

Rect EditorState::GetEditorSceneViewportRect()
{
    Vec2f ViewportSize = Engine::GetClientAreaSize();

    Rect SceneViewportRect = Rect(  Vec2f(80.0f, 60.0f), 
                                    Vec2f(ViewportSize.x - 520.0f, ViewportSize.y * 0.7f - 60.0f));
    return SceneViewportRect;
}

Ray EditorState::GetMouseRay(Camera& cam, Vec2i mousePosition, Rect viewPort)
{
    Mat4x4f invCamMatrix = cam.GetInvCamMatrix();

    Vec3f a, b;

    Vec2i screenSize = viewPort.size;

    Vec2i mousePos;
    mousePos.x = mousePosition.x - (int)viewPort.location.x;
    mousePos.y = mousePosition.y - (int)viewPort.location.y;

    float x = ((2.0f * (float)mousePos.x) / (float)screenSize.x) - 1.0f;
    float y = 1.0f - ((2.0f * (float)mousePos.y) / (float)screenSize.y);
    float z = 1.0f;

    Vec3f ray_nds = Vec3f(x, y, z);

    Vec4f ray_clip = Vec4f(ray_nds.x, ray_nds.y, -1.0f, 1.0f);

    Vec4f ray_eye = ray_clip * Math::inv(cam.GetProjectionMatrix());

    ray_eye = Vec4f(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

    Vec4f ray_world = ray_eye * Math::inv(cam.GetViewMatrix());

    Vec3f ray = Vec3f(ray_world.x, ray_world.y, ray_world.z);
    ray = Math::normalize(ray);

    return Ray(cam.GetPosition(), ray);
}

void EditorState::LoadEditorResources()
{
    GraphicsModule* Graphics = GraphicsModule::Get();
    TextModule* Text = TextModule::Get();

    AssetRegistry* Registry = AssetRegistry::Get();

    // Load textures needed for editor gizmos
    Texture RedTexture = *Registry->LoadTexture("textures/red.png");
    Texture GreenTexture = *Registry->LoadTexture("textures/green.png");
    Texture BlueTexture = *Registry->LoadTexture("textures/blue.png");
    Texture PurpleTexture = *Registry->LoadTexture("textures/purple.png");

    WhiteMaterial = Graphics->CreateMaterial(*Registry->LoadTexture("images/white.png"));

    // Create translation gizmo models
    xAxisArrow = Graphics->CreateModel(TexturedMesh(
        *Registry->LoadStaticMesh("models/ArrowSmooth.obj"),
        Graphics->CreateMaterial(RedTexture)
    ));

    yAxisArrow = Graphics->CloneModel(xAxisArrow);
    zAxisArrow = Graphics->CloneModel(xAxisArrow);
    
    xAxisArrow.GetTransform().SetScale(Vec3f(0.2f, 1.0f, 0.2f));
    yAxisArrow.GetTransform().SetScale(Vec3f(0.2f, 1.0f, 0.2f));
    zAxisArrow.GetTransform().SetScale(Vec3f(0.2f, 1.0f, 0.2f));

    xAxisArrow.GetTransform().Rotate(Quaternion(Vec3f(0.0f, 0.0f, 1.0f), -(float)M_PI_2));
    zAxisArrow.GetTransform().Rotate(Quaternion(Vec3f(1.0f, 0.0f, 0.0f), (float)M_PI_2));

    yAxisArrow.SetMaterial(Graphics->CreateMaterial(GreenTexture));
    zAxisArrow.SetMaterial(Graphics->CreateMaterial(BlueTexture));

    TranslateBall = Graphics->CreateModel(TexturedMesh(
        *Registry->LoadStaticMesh("models/Buckyball.obj"),
        Graphics->CreateMaterial(PurpleTexture)
    ));

    // Create rotation gizmo models
    xAxisRing = Graphics->CreateModel(TexturedMesh(
        *Registry->LoadStaticMesh("models/RotationHoop.obj"),
        Graphics->CreateMaterial(RedTexture)
    ));

    yAxisRing = Graphics->CloneModel(xAxisRing);
    zAxisRing = Graphics->CloneModel(xAxisRing);

    xAxisRing.GetTransform().Rotate(Quaternion(Vec3f(0.0f, 0.0f, 1.0f), -(float)M_PI_2));
    zAxisRing.GetTransform().Rotate(Quaternion(Vec3f(1.0f, 0.0f, 0.0f), (float)M_PI_2));

    yAxisRing.SetMaterial(Graphics->CreateMaterial(GreenTexture));
    zAxisRing.SetMaterial(Graphics->CreateMaterial(BlueTexture));

    // Create scaling gizmo models
    xScaleWidget = Graphics->CreateModel(TexturedMesh(
        *Registry->LoadStaticMesh("models/ScaleWidget.obj"),
        Graphics->CreateMaterial(RedTexture)
    ));

    yScaleWidget = Graphics->CloneModel(xScaleWidget);
    zScaleWidget = Graphics->CloneModel(xScaleWidget);

    xScaleWidget.GetTransform().Rotate(Quaternion(Vec3f(0.0f, 1.0f, 0.0f), (float)M_PI_2));
    yScaleWidget.GetTransform().Rotate(Quaternion(Vec3f(1.0f, 0.0f, 0.0f), -(float)M_PI_2));

    xScaleWidget.GetTransform().SetScale(Vec3f(0.4f, 0.4f, 0.8f));
    yScaleWidget.GetTransform().SetScale(Vec3f(0.4f, 0.4f, 0.8f));
    zScaleWidget.GetTransform().SetScale(Vec3f(0.4f, 0.4f, 0.8f));

    yScaleWidget.SetMaterial(Graphics->CreateMaterial(GreenTexture));
    zScaleWidget.SetMaterial(Graphics->CreateMaterial(BlueTexture));

    // Load editor UI textures
    playButtonTexture = *Registry->LoadTexture("images/playButton.png");

    modelSelectToolTexture = *Registry->LoadTexture("images/cursorTool.png");
    vertexSelectToolTexture = *Registry->LoadTexture("images/vertexSelectTool.png");

    boxToolTexture = *Registry->LoadTexture("images/boxTool.png");
    planeToolTexture = *Registry->LoadTexture("images/planeTool.png");

    translateToolTexture = *Registry->LoadTexture("images/translateTool.png");
    rotateToolTexture = *Registry->LoadTexture("images/rotateTool.png");
    scaleToolTexture = *Registry->LoadTexture("images/scaleTool.png");

    vertexToolTexture = *Registry->LoadTexture("images/vertexTool.png");
    sculptToolTexture = *Registry->LoadTexture("images/sculptTool.png");
    
    lightEntityTexture = *Registry->LoadTexture("images/lightTool.png");
    directionalLightEntityTexture = *Registry->LoadTexture("images/dirLight.png");
    cameraEntityTexture = *Registry->LoadTexture("images/cameraButton.png");
    brainEntityTexture = *Registry->LoadTexture("images/brainTool.png");
    billboardEntityTexture = *Registry->LoadTexture("images/billboardTool.png");

    // Load editor fonts
    DefaultFont = Text->LoadFont("fonts/ARLRDBD.TTF", 30);
    InspectorFont = Text->LoadFont("fonts/ARLRDBD.TTF", 15);
}

std::vector<Model> EditorState::LoadModels(GraphicsModule& graphics)
{
    AssetRegistry* Registry = AssetRegistry::Get();
    
    std::vector<Model> LoadedModels;

    std::string path = "models";

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        std::filesystem::path ext = entry.path().extension();
        if (ext.string() == ".obj")
        {
            std::string fileName = entry.path().generic_string();

            StaticMesh newMesh = *Registry->LoadStaticMesh(fileName);
            
            // For now we load all models with a temporary white texture
            Model newModel = graphics.CreateModel(TexturedMesh(newMesh, WhiteMaterial));

            LoadedModels.push_back(newModel);
        }
    }

    return LoadedModels;
}

std::vector<Material> EditorState::LoadMaterials(GraphicsModule& graphics)
{
    std::clock_t start;
    double duration;

    start = std::clock();

    AssetRegistry* Registry = AssetRegistry::Get();

    std::vector<Material> LoadedMaterials;

    std::string path = "textures";

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        std::filesystem::path ext = entry.path().extension();
        std::string extensionString = ext.string();
        if (extensionString == ".png" || extensionString == ".jpg")
        {
            std::string fileName = entry.path().generic_string();

            if (StringUtils::Contains(fileName, ".norm."))
            {
                continue;
            }
            if (StringUtils::Contains(fileName, ".metal."))
            {
                continue;
            }
            if (StringUtils::Contains(fileName, ".rough."))
            {
                continue;
            }
            if (StringUtils::Contains(fileName, ".ao."))
            {
                continue;
            }

            Engine::DEBUGPrint(fileName);

            Texture newTexture = *Registry->LoadTexture(fileName);

            std::string NormalMapString = entry.path().parent_path().generic_string() + "/" + entry.path().stem().generic_string() + ".norm" + extensionString;
            std::string RoughnessMapString = entry.path().parent_path().generic_string() + "/" + entry.path().stem().generic_string() + ".rough" + extensionString;
            std::string MetallicMapString = entry.path().parent_path().generic_string() + "/" + entry.path().stem().generic_string() + ".metal" + extensionString;
            std::string AOMapString = entry.path().parent_path().generic_string() + "/" + entry.path().stem().generic_string() + ".ao" + extensionString;

            if (std::filesystem::exists(NormalMapString))
            {
                if (std::filesystem::exists(RoughnessMapString))
                {
                    if (std::filesystem::exists(MetallicMapString))
                    {
                        if (std::filesystem::exists(AOMapString))
                        {
                            Texture newNormal = *Registry->LoadTexture(NormalMapString);
                            Texture newRoughness = *Registry->LoadTexture(RoughnessMapString);
                            Texture newMetal = *Registry->LoadTexture(MetallicMapString);
                            Texture newAO = *Registry->LoadTexture(AOMapString);
                            LoadedMaterials.push_back(graphics.CreateMaterial(newTexture, newNormal, newRoughness, newMetal, newAO));
                        }
                        else
                        {
                            Texture newNormal = *Registry->LoadTexture(NormalMapString);
                            Texture newRoughness = *Registry->LoadTexture(RoughnessMapString);
                            Texture newMetal = *Registry->LoadTexture(MetallicMapString);
                            LoadedMaterials.push_back(graphics.CreateMaterial(newTexture, newNormal, newRoughness, newMetal));
                        }
                    }
                    else
                    {
                        Texture newNormal = *Registry->LoadTexture(NormalMapString);
                        Texture newRoughness = *Registry->LoadTexture(RoughnessMapString);
                        LoadedMaterials.push_back(graphics.CreateMaterial(newTexture, newNormal, newRoughness));
                    }
                }
                else
                {
                    Texture newNormal = *Registry->LoadTexture(NormalMapString);
                    LoadedMaterials.push_back(graphics.CreateMaterial(newTexture, newNormal));
                }
            }
            else
            {
                LoadedMaterials.push_back(graphics.CreateMaterial(newTexture));
            }

        }
    }

    duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;

    Engine::DEBUGPrint("Took " + std::to_string(duration) + " seconds to load all textures.");

    return LoadedMaterials;

}

void EditorState::MoveCamera(Camera* Camera, float PixelToRadians, double DeltaTime)
{
    InputModule* Input = InputModule::Get();

    const float CamSpeed = 10.0f;

    float Speed = CamSpeed * (float)DeltaTime;

    if (Input->IsKeyDown(Key::Shift))
    {
        Speed *= 5.0f;
    }

    if (Input->IsKeyDown(Key::W))
    {
        Camera->Move(Camera->GetDirection() * Speed);
    }
    if (Input->IsKeyDown(Key::S))
    {
        Camera->Move(-Camera->GetDirection() * Speed);
    }
    if (Input->IsKeyDown(Key::D))
    {
        Camera->Move(Math::normalize(Camera->GetPerpVector()) * Speed);
    }
    if (Input->IsKeyDown(Key::A))
    {
        Camera->Move(-Math::normalize(Camera->GetPerpVector()) * Speed);
    }

    if (Input->IsKeyDown(Key::Space))
    {
        Camera->Move(Vec3f(0.0f, 0.0f, Speed));
    }
    if (Input->IsKeyDown(Key::Ctrl))
    {
        Camera->Move(Vec3f(0.0f, 0.0f, -Speed));
    }

    Camera->RotateCamBasedOnDeltaMouse(Input->GetMouseState().GetDeltaMousePos(), PixelToRadians);
}

void EditorState::DrawLevelEditor(GraphicsModule* Graphics, UIModule* UI, float DeltaTime)
{
    Vec2i ViewportSize = Engine::GetClientAreaSize();

    auto BrushVec = EditorScene.GetBrushes();

    for (Brush* B : BrushVec)
    {
        B->UpdatedThisFrame = false;
        for (auto& Face : B->Faces)
        {
            Vec3f PlanePoint = Vec3f(0.0f, 0.0f, 0.0f);

            for (int i = 0; i < Face.size(); i++)
            {
                PlanePoint += B->Vertices[Face[i]];
            }
            PlanePoint = PlanePoint / (float)Face.size();

            for (int i = 0; i < Face.size() - 1; i++)
            {
                //Graphics->DebugDrawLine(*Face[i], *Face[i + 1]);
            }
            //Graphics->DebugDrawLine(*Face[Face.size() - 1], *Face[0]);

            Vec3f u = B->Vertices[Face[1]] - B->Vertices[Face[0]];
            Vec3f v = B->Vertices[Face[2]] - B->Vertices[Face[0]];

            Vec3f PlaneNorm = -Math::cross(u, v);
            PlaneNorm = Math::normalize(PlaneNorm);

            Vec3f UpProjection = Math::ProjectVecOnPlane(Vec3f(0.0f, 0.0f, 1.0f), Plane(PlanePoint, PlaneNorm));

            if (UpProjection.IsNearlyZero())
            {
                if (PlaneNorm.z > 0.0f)
                {
                    UpProjection = u;
                }
                else
                {
                    UpProjection = -u;
                }
            }

            UpProjection = Math::normalize(UpProjection);

            //Graphics->DebugDrawLine(PlanePoint, PlanePoint + UpProjection, c_VegasGold);

            //Graphics->DebugDrawLine(PlanePoint, PlanePoint + PlaneNorm, c_LightGoldenRodYellow);
        }

    }

    EditorScene.EditorDraw(*Graphics, ViewportBuffer, &ViewportCamera);
    
    Graphics->SetActiveFrameBuffer(WidgetBuffer);
    {
        Graphics->SetCamera(&ViewportCamera);

        Graphics->SetRenderMode(RenderMode::FULLBRIGHT);

        Cursor.DrawTransientModels();
    }
    Graphics->ResetFrameBuffer();

    UI->BufferPanel(ViewportBuffer.FinalOutput, GetEditorSceneViewportRect());

    UI->BufferPanel(WidgetBuffer, GetEditorSceneViewportRect());

    TextModule::Get()->DrawText("Frame Time: " + std::to_string(DeltaTime), &TestFont, GetEditorSceneViewportRect().location);

    PrevFrameTimeCount++;
    PrevFrameTimeSum += DeltaTime;

    if (PrevFrameTimeSum > 0.5f)
    {
        PrevAveFPS = (int)round(1.0f / (PrevFrameTimeSum / PrevFrameTimeCount));
        PrevFrameTimeCount = 0;
        PrevFrameTimeSum -= 0.5f;
    }

    TextModule::Get()->DrawText("FPS: " + std::to_string(PrevAveFPS), &TestFont, GetEditorSceneViewportRect().location + Vec2f(0.0f, 30.0f));

    DrawEditorUI();
}

void EditorState::DrawEditorUI()
{
    GraphicsModule* Graphics = GraphicsModule::Get();
    UIModule* UI = UIModule::Get();

    Vec2i ScreenSize = Engine::GetClientAreaSize();
    Rect ViewportRect = GetEditorSceneViewportRect();

    // Left toolbar buttons
    Rect ToolbarButtonRect = Rect(Vec2f(0.0f, 60.0f), Vec2f(ViewportRect.location.x, ViewportRect.size.y));

    UI->StartFrame("Tools", ToolbarButtonRect, 0.0f, c_FrameLight);
    {
        Vec3f SelectedColour = c_SelectedButton;
        Vec3f UnSelectedColour = c_Button;

        Texture SelectModeTexture = modelSelectToolTexture;
        switch (Cursor.GetSelectMode())
        {
        case SelectMode::ModelSelect:
            SelectModeTexture = modelSelectToolTexture;
            break;
        case SelectMode::VertexSelect:
            SelectModeTexture = vertexSelectToolTexture;
            break;
        default:
            break;
        }

        if (UI->ImgButton("SelectTool", SelectModeTexture, Vec2f(80.0f, 80.0f), 12.0f,
            Cursor.GetToolMode() == ToolMode::Select ? SelectedColour : UnSelectedColour))
        {
            if (Cursor.GetToolMode() == ToolMode::Select)
            {
                Cursor.CycleSelectMode();
            }
            else
            {
                Cursor.SetToolMode(ToolMode::Select);
            }
        }

        Texture TransModeTexture = translateToolTexture;
        switch (Cursor.GetTransMode())
        {
        case TransformMode::Translate:
            TransModeTexture = translateToolTexture;
            break;
        case TransformMode::Rotate:
            TransModeTexture = rotateToolTexture;
            break;
        case TransformMode::Scale:
            TransModeTexture = scaleToolTexture;
            break;
        default:
            break;
        }

        if (UI->ImgButton("TransformTool", TransModeTexture, Vec2f(80.0f, 80.0f), 12.0f,
            Cursor.GetToolMode() == ToolMode::Transform ? SelectedColour : UnSelectedColour))
        {
            if (Cursor.GetToolMode() == ToolMode::Transform)
            {
                Cursor.CycleTransformMode();
            }
            else
            {
                Cursor.SetToolMode(ToolMode::Transform);
            }
        }

        Texture GeoModeTexture = boxToolTexture;
        switch (Cursor.GetGeoMode())
        {
        case GeometryMode::Box:
            GeoModeTexture = boxToolTexture;
            break;
        case GeometryMode::Plane:
            GeoModeTexture = planeToolTexture;
            break;
        default:
            break;
        }

        if (UI->ImgButton("GeometryTool", GeoModeTexture, Vec2f(80.0f, 80.0f), 12.0f,
            Cursor.GetToolMode() == ToolMode::Geometry ? SelectedColour : UnSelectedColour))
        {
            if (Cursor.GetToolMode() == ToolMode::Geometry)
            {
                Cursor.CycleGeometryMode();
            }
            else
            {
                Cursor.SetToolMode(ToolMode::Geometry);
            }
        }

        if (UI->ImgButton("SculptTool", sculptToolTexture, Vec2f(80.0f, 80.0f), 12.0f,
            Cursor.GetToolMode() == ToolMode::Sculpt ? SelectedColour : UnSelectedColour))
        {
            Cursor.SetToolMode(ToolMode::Sculpt);
        }

        if (UI->ImgButton("BrushTool", vertexToolTexture, Vec2f(80.0f, 80.0f), 12.0f,
            Cursor.GetToolMode() == ToolMode::Brush ? SelectedColour : UnSelectedColour))
        {
            Cursor.SetToolMode(ToolMode::Brush);
        }


    }
    UI->EndFrame();

    DrawTopPanel();
    DrawDrawerSettingsPanel();
    DrawResourcesPanel();
    DrawInspectorPanel();
}

void EditorState::DrawTopPanel()
{
    GraphicsModule* Graphics = GraphicsModule::Get();
    UIModule* UI = UIModule::Get();

    Vec2i ScreenSize = Engine::GetClientAreaSize();
    Rect ViewportRect = GetEditorSceneViewportRect();

    Rect TopPanelRect = Rect(Vec2f(0.0f, 20.0f),
        Vec2f((float)ScreenSize.x, ViewportRect.location.y - 20.0f));

    //Vec2f TopPanelButtonSize = Vec2f(ViewportRect.location.y, ViewportRect.location.y);

    UI->StartFrame("Top", TopPanelRect, 0.0f, c_FrameDark);
    {
        if (UI->ImgButton("PlayButton", playButtonTexture, Vec2f(40.0f, 40.0f), 8.0f, c_TopButton))
        {
            Cursor.UnselectAll();
            Cursor.ResetAllState();
            GameState* NewGameState = new GameState();
            NewGameState->LoadScene(EditorScene);

            Machine->PushState(NewGameState);
        }
        if (UI->TextButton("New", Vec2f(40.0f, 40.0f), 8.0f, c_TopButton, Vec3f(1.0f, 1.0f, 1.0f)))
        {
            Cursor.UnselectAll();
            EditorScene.Clear();
            BehaviourRegistry::Get()->ClearAllAttachedBehaviours();
        }
        if (UI->TextButton("Open", Vec2f(40.0f, 40.0f), 8.0f, c_TopButton, Vec3f(1.0f, 1.0f, 1.0f)))
        {
            Cursor.UnselectAll();
            std::string FileName;
            if (Engine::FileOpenDialog(FileName))
            {
                EditorScene.Load(FileName);
            }
        }
        if (UI->TextButton("Save", Vec2f(40.0f, 40.0f), 8.0f, c_TopButton, Vec3f(1.0f, 1.0f, 1.0f)))
        {
            std::string FileName;
            if (Engine::FileSaveDialog(FileName))
            {
                EditorScene.Save(FileName);
            }
        }
    }
    UI->EndFrame();
}

void EditorState::DrawDrawerSettingsPanel()
{
    UIModule* UI = UIModule::Get();

    Vec2i ScreenSize = Engine::GetClientAreaSize();
    Rect ViewportRect = GetEditorSceneViewportRect();

    Rect ModeSelectRect = Rect(
        Vec2f(0.0f, ViewportRect.location.y + ViewportRect.size.y), 
        Vec2f(ViewportRect.location.x, ScreenSize.y - (ViewportRect.location.y + ViewportRect.size.y))
    );

    UI->StartFrame("Mode Select", ModeSelectRect, 6.0f, c_FrameLight);
    {
        if (UI->TextButton("Content", Vec2f(ModeSelectRect.size.x - 16.0f, 40.0f), 6.0f))
        {
            Drawer = DrawerMode::CONTENT;
        }
        if (UI->TextButton("Browser", Vec2f(ModeSelectRect.size.x - 16.0f, 40.0f), 6.0f))
        {
            Drawer = DrawerMode::BROWSER;
        }

    }
    UI->EndFrame();
}

void EditorState::DrawResourcesPanel()
{
    GraphicsModule* Graphics = GraphicsModule::Get();
    UIModule* UI = UIModule::Get();

    // Draw meshes
    Graphics->SetCamera(&ModelCamera);

    Vec2i ScreenSize = Engine::GetClientAreaSize();
    Rect ViewportRect = GetEditorSceneViewportRect();

    Rect ResourcePanelRect = Rect(Vec2f(ViewportRect.location.x, ViewportRect.location.y + ViewportRect.size.y), Vec2f(ViewportRect.size.x, ScreenSize.y - (ViewportRect.location.y + ViewportRect.size.y)));

    if (Drawer == DrawerMode::CONTENT)
    {
        UI->StartFrame("Resources", ResourcePanelRect, 16.0f, c_FrameDark);
        {
            UI->StartTab("Models", c_Tab);
            {
                int index = 0;
                for (auto& AModel : LoadedModels)
                {
                    if (UI->TextButton(AModel.m_TexturedMeshes[0].m_Mesh.Path.GetFileNameNoExt(), Vec2f(120.0f, 40.0f), 10.0f, c_ResourceButton).clicking)
                    {
                        if (!Cursor.IsDraggingSomething())
                        {
                            Model* AddedModel = EditorScene.AddModel(new Model(AModel));
                            Cursor.StartDraggingNewModel(AddedModel);
                        }
                    }
                    index++;
                }
            }
            UI->EndTab();

            UI->StartTab("Materials", c_Tab);
            {
                for (auto& Mat : LoadedMaterials)
                {
                    if (UI->ImgButton(Mat.m_Albedo.Path.GetFileNameNoExt(), Mat.m_Albedo, Vec2f(80, 80), 5.0f, c_ResourceButton).clicking)
                    {
                        if (!Cursor.IsDraggingSomething())
                        {
                            Material* MatPtr = &Mat;
                            Cursor.StartDraggingNewMaterial(MatPtr);
                        }

                    }
                }
            }
            UI->EndTab();

            UI->StartTab("Entities", c_Tab);
            {
                if (UI->ImgButton("LightEntity", lightEntityTexture, Vec2f(80.0f, 80.0f), 12.0f, c_ResourceButton).clicking)
                {
                    if (!Cursor.IsDraggingSomething())
                    {
                        PointLight NewLight = PointLight();

                        PointLight* PointLightPtr = EditorScene.AddPointLight(NewLight);
                        Cursor.StartDraggingNewPointLight(PointLightPtr);
                    }
                }
                if (UI->ImgButton("DirectionalLightEntity", directionalLightEntityTexture, Vec2f(80.0f, 80.0f), 12.0f, c_ResourceButton).clicking)
                {

                }
                if (UI->ImgButton("CameraEntity", cameraEntityTexture, Vec2f(80.0f, 80.0f), 12.0f, c_ResourceButton))
                {
                }
                if (UI->ImgButton("BrainEntity", brainEntityTexture, Vec2f(80.0f, 80.0f), 12.0f, c_ResourceButton))
                {
                }
                if (UI->ImgButton("BillboardEntity", billboardEntityTexture, Vec2f(80.0f, 80.0f), 12.0f, c_ResourceButton))
                {
                }
            }
            UI->EndTab();

            UI->StartTab("Behaviours", c_Tab);
            {
                auto BehaviourMap = BehaviourRegistry::Get()->GetBehaviours();

                for (auto Behaviour : BehaviourMap)
                {
                    if (UI->TextButton(Behaviour.first, Vec2f(120, 40), 10.0f, c_ResourceButton).clicking)
                    {
                        if (!Cursor.IsDraggingSomething())
                        {
                            Cursor.StartDraggingNewBehaviour(Behaviour.first);
                        }
                    }
                }
            }
            UI->EndTab();

            UI->StartTab("1000 Buttons", c_Tab);
            {
                for (int i = 0; i < 1000; ++i)
                {
                    UI->TextButton("", Vec2f(80.0f, 80.0f), 4.0f, RandomColours[i]);
                }
            }
            UI->EndTab();

            UI->StartTab("Experiment", c_Tab);
            {
            }
            UI->EndTab();

        }
        UI->EndFrame();
    }
    else if (Drawer == DrawerMode::BROWSER)
    {
        UI->StartFrame("Browser", ResourcePanelRect, 16.0f, c_FrameDark);
        {
            if (UI->TextButton("..", Vec2f(120.0f, 40.0f), 8.0f, c_NiceYellow))
            {
                CurrentResourceDirectoryPath = CurrentResourceDirectoryPath.parent_path();
            }

            for (const auto& entry : std::filesystem::directory_iterator(CurrentResourceDirectoryPath))
            {
                if (entry.is_directory())
                {
                    if (UI->TextButton(entry.path().filename().generic_string(), Vec2f(120.0f, 80.0f), 8.0f, c_FrameLight))
                    {
                        CurrentResourceDirectoryPath = entry.path();
                        break;
                    }
                }
                else
                {
                    if (UI->TextButton(entry.path().filename().generic_string(), Vec2f(120.0f, 80.0f), 8.0f, c_ResourceButton))
                    {

                    }
                }
            }
        }
        UI->EndFrame();
    }

}

void EditorState::DrawInspectorPanel()
{
    GraphicsModule* Graphics = GraphicsModule::Get();
    UIModule* UI = UIModule::Get();

    Vec2i ScreenSize = Engine::GetClientAreaSize();
    Rect ViewportRect = GetEditorSceneViewportRect();

    Rect InspectorPanelRect = Rect( Vec2f(ViewportRect.location.x + ViewportRect.size.x, ViewportRect.location.y), 
                                    Vec2f(ScreenSize.x - (ViewportRect.location.x + ViewportRect.size.x), ScreenSize.y - ViewportRect.location.y));

    UI->StartFrame("Inspector", InspectorPanelRect, 16.0f, c_Inspector);
    {
        Cursor.DrawInspectorPanel();
    }
    UI->EndFrame();

}
