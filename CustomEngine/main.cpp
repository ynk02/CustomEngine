#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cfloat>
#include <functional>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "CoreMinimal.h"
#include "UObject.h"
#include "UActorComponent.h"
#include "USceneComponent.h"
#include "UPrimitiveComponent.h"
#include "UStaticMeshComponent.h"
#include "AActor.h"
#include "ACubeActor.h"
#include "AFloorActor.h"
#include "FCamera.h"
#include "FRay.h"
#include "UShader.h"
#include "URigidBodyComponent.h"

#include "Physics/IPhysicsSystem.h"
#include "Physics/IPhysicsScene.h"
#include "Physics/IPhysicsActor.h"

#include "Physics/XPBDPhysicsSystem.h"
#include "Physics/AVBDPhysicsSystem.h"

#include "Editor/UWorldOutliner.h"
#include "Editor/UDetailsPanel.h"

#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

static constexpr float PHYSICS_FIXED_STEP = 1.0f / 60.0f;

class UApplication : public UObject
{
public:
    UApplication(int width, int height, const std::string& title)
        : m_Width(width), m_Height(height), m_Title(title)
        , m_Window(nullptr)
        , FBO(0), TextureColor(0), RBO(0)
        , TexWidth(1), TexHeight(1)
    {}

    ~UApplication() { Shutdown(); }

    bool Init()
    {
        if (!InitGLFW())  return false;
        if (!InitGLAD())  return false;
        InitImGui();

        Shader = UShader::CreateFromFiles("Shaders/Shader.vert", "Shaders/Shader.frag");
        CreateFBO(m_Width, m_Height);

        glfwSetWindowUserPointer(m_Window, this);
        glfwSetWindowRefreshCallback(m_Window, [](GLFWwindow* w) {
            auto* app = static_cast<UApplication*>(glfwGetWindowUserPointer(w));
            if (app) app->RenderOneFrame(0.016f);
        });

        CurrentBackend = EPhysicsBackend::Null;
        PhysicsSystem  = IPhysicsSystem::Create(CurrentBackend);
        PhysicsSystem->Initialize();

        PhysicsScene = PhysicsSystem->CreateScene();
        PhysicsScene->Initialize(FVector(0.0f, -9.81f, 0.0f));

        WorldOutliner = std::make_unique<UWorldOutliner>();
        DetailsPanel  = std::make_unique<UDetailsPanel>();

        auto Floor = MakeShared<AFloorActor>();
        Floor->SetName("Floor");
        Floor->SetTransform(FTransform(FVector(0,0,0), FRotator(0,0,0), FVector(1,1,1)));
        WorldActors.push_back(Floor);

        SpawnCube("Cube_0", FVector(0.0f, 3.0f, 0.0f));

        LastFrameTime = (float)glfwGetTime();

        for (auto& Actor : WorldActors)
            if (Actor) Actor->BeginPlay();

        Cam.UpdateVectors();
        return true;
    }

    void Run()
    {
        while (!glfwWindowShouldClose(m_Window))
        {
            float now = (float)glfwGetTime();
            float dt  = glm::clamp(now - LastFrameTime, 0.0f, 0.1f);
            LastFrameTime = now;

            glfwPollEvents();

            if (bPendingBackendSwitch)
            {
                DoSwitchPhysicsBackend(PendingBackend);
                bPendingBackendSwitch = false;
            }

            PhysicsAccumulator += dt;
            while (PhysicsAccumulator >= PHYSICS_FIXED_STEP)
            {
                PhysicsScene->Simulate(PHYSICS_FIXED_STEP);
                PhysicsAccumulator -= PHYSICS_FIXED_STEP;
            }

            for (auto& Actor : WorldActors)
                if (Actor) Actor->Tick(dt);

            RenderOneFrame(dt);
        }
    }

    void RenderOneFrame(float dt)
    {
        BeginFrame();
        RenderUI(dt);
        EndFrame();
    }

private:

    void SpawnCube(const std::string& Name, const FVector& Location)
    {
        auto Cube = MakeShared<ACubeActor>();
        Cube->SetName(Name);

        FTransform InitialTransform(Location, FRotator(0,0,0), FVector(1,1,1));
        Cube->SetTransform(InitialTransform);

        if (URigidBodyComponent* RB = Cube->GetRigidBodyComponent())
        {
            RB->SetHalfExtents(FVector(0.5f));
            RB->RegisterWithPhysicsScene(PhysicsScene.get(), InitialTransform);
        }

        WorldActors.push_back(Cube);
        Cube->BeginPlay();
    }

    void RequestSwitchPhysicsBackend(EPhysicsBackend NewBackend)
    {
        if (NewBackend == CurrentBackend) return;
        PendingBackend        = NewBackend;
        bPendingBackendSwitch = true;
    }

    void DoSwitchPhysicsBackend(EPhysicsBackend NewBackend)
    {

        for (auto& Actor : WorldActors)
        {
            if (!Actor) continue;
            for (auto& Comp : Actor->GetComponents())
            {
                if (auto RB = Cast<URigidBodyComponent>(Comp))
                    RB->DetachFromPhysicsScene();
            }
        }

        PhysicsScene.reset();
        PhysicsSystem->Shutdown();
        PhysicsSystem = IPhysicsSystem::Create(NewBackend);
        PhysicsSystem->Initialize();
        PhysicsScene = PhysicsSystem->CreateScene();

        FVector Gravity = FVector(0.0f, WorldGravityY, 0.0f);
        PhysicsScene->Initialize(Gravity);

        for (auto& Actor : WorldActors)
        {
            if (!Actor) continue;
            if (URigidBodyComponent* RB = dynamic_cast<URigidBodyComponent*>(
                    Actor->GetRootComponent()))
            {
                RB->RegisterWithPhysicsScene(PhysicsScene.get(),
                                             Actor->GetTransform());
            }
        }

        CurrentBackend = NewBackend;
        PhysicsAccumulator = 0.0f;
    }

    FRay ComputeRayFromMouse(float mouseX, float mouseY, int viewW, int viewH) const
    {
        float nx =  (mouseX / viewW) * 2.0f - 1.0f;
        float ny = -(mouseY / viewH) * 2.0f + 1.0f;

        glm::vec4 rayClip  = glm::vec4(nx, ny, -1.0f, 1.0f);
        glm::vec4 rayEye   = glm::inverse(LastProjection) * rayClip;
        rayEye             = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
        glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(LastView) * rayEye));

        return { Cam.Position, rayWorld };
    }

    void TrySelectActorAtMouse(float mouseX, float mouseY, int viewW, int viewH)
    {
        FRay Ray = ComputeRayFromMouse(mouseX, mouseY, viewW, viewH);

        FRaycastHit Hit = PhysicsScene->Raycast(Ray, 1000.0f);

        if (Hit.bHit && Hit.HitActor)
        {

            int FoundIdx = FindActorByPhysicsActor(Hit.HitActor);
            if (FoundIdx >= 0)
            {
                SelectedActorIndex = FoundIdx;
                return;
            }
        }

        SelectedActorIndex = RaycastAABBFallback(Ray);
    }

    int FindActorByPhysicsActor(IPhysicsActor* Target) const
    {
        for (int i = 0; i < (int)WorldActors.size(); ++i)
        {
            if (!WorldActors[i]) continue;
            for (const auto& Comp : WorldActors[i]->GetComponents())
            {
                if (auto RB = Cast<URigidBodyComponent>(Comp))
                {
                    if (RB->GetPhysicsActor() == Target)
                        return i;
                }
            }
        }
        return -1;
    }

    int RaycastAABBFallback(const FRay& Ray) const
    {
        float ClosestT  = FLT_MAX;
        int   ClosestIdx = -1;

        for (int i = 0; i < (int)WorldActors.size(); ++i)
        {
            const auto& Actor = WorldActors[i];
            if (!Actor) continue;

            FTransform T    = Actor->GetTransform();
            FVector    Half = T.Scale3D * 0.5f;
            FVector    Mn   = T.Location - Half;
            FVector    Mx   = T.Location + Half;

            float tMin = -FLT_MAX, tMax = FLT_MAX;
            bool  bHit = true;

            for (int Axis = 0; Axis < 3; ++Axis)
            {
                if (std::abs(Ray.Direction[Axis]) < 1e-6f)
                {
                    if (Ray.Origin[Axis] < Mn[Axis] || Ray.Origin[Axis] > Mx[Axis])
                    { bHit = false; break; }
                }
                else
                {
                    float t1 = (Mn[Axis] - Ray.Origin[Axis]) / Ray.Direction[Axis];
                    float t2 = (Mx[Axis] - Ray.Origin[Axis]) / Ray.Direction[Axis];
                    if (t1 > t2) std::swap(t1, t2);
                    tMin = std::max(tMin, t1);
                    tMax = std::min(tMax, t2);
                    if (tMin > tMax) { bHit = false; break; }
                }
            }

            if (bHit)
            {
                float tHit = (tMin > 0.0f) ? tMin : tMax;
                if (tHit > 0.0f && tHit < ClosestT)
                {
                    ClosestT   = tHit;
                    ClosestIdx = i;
                }
            }
        }
        return ClosestIdx;
    }

    void BeginActorDrag(float mouseX, float mouseY, int viewW, int viewH)
    {
        if (SelectedActorIndex < 0 || SelectedActorIndex >= (int)WorldActors.size()) return;
        auto& Actor = WorldActors[SelectedActorIndex];
        if (!Actor) return;

        FRay ray   = ComputeRayFromMouse(mouseX, mouseY, viewW, viewH);
        DragPlaneY = Actor->GetTransform().Location.y;

        if (std::abs(ray.Direction.y) > 1e-6f)
        {
            float t = (DragPlaneY - ray.Origin.y) / ray.Direction.y;
            if (t > 0.0f)
            {
                DragHitOffset    = ray.Origin + t * ray.Direction - Actor->GetTransform().Location;
                bIsDraggingActor = true;
            }
        }
    }

    void UpdateActorDrag(float mouseX, float mouseY, int viewW, int viewH)
    {
        if (!bIsDraggingActor || SelectedActorIndex < 0) return;
        auto& Actor = WorldActors[SelectedActorIndex];
        if (!Actor) return;

        FRay ray = ComputeRayFromMouse(mouseX, mouseY, viewW, viewH);
        if (std::abs(ray.Direction.y) > 1e-6f)
        {
            float t = (DragPlaneY - ray.Origin.y) / ray.Direction.y;
            if (t > 0.0f)
            {
                FVector NewLoc  = ray.Origin + t * ray.Direction - DragHitOffset;
                NewLoc.y        = DragPlaneY;
                FTransform Trans = Actor->GetTransform();
                Trans.Location   = NewLoc;

                for (const auto& Comp : Actor->GetComponents())
                {
                    if (auto RB = Cast<URigidBodyComponent>(Comp))
                    {
                        RB->SyncTransformToPhysics(Trans);
                        return;
                    }
                }
                Actor->SetTransform(Trans);
            }
        }
    }

    bool InitGLFW()
    {
        if (!glfwInit()) return false;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), NULL, NULL);
        if (!m_Window) { glfwTerminate(); return false; }
        glfwMakeContextCurrent(m_Window);
        glfwSwapInterval(1);
        return true;
    }

    bool InitGLAD()
    {
        return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) != 0;
    }

    void InitImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        ImGui::StyleColorsDark();

        ImGuiStyle& Style = ImGui::GetStyle();
        Style.WindowRounding   = 6.0f;
        Style.FrameRounding    = 4.0f;
        Style.PopupRounding    = 4.0f;
        Style.ScrollbarRounding= 4.0f;
        Style.GrabRounding     = 4.0f;
        Style.TabRounding      = 4.0f;
        Style.FramePadding     = ImVec2(6.0f, 4.0f);
        Style.ItemSpacing      = ImVec2(8.0f, 6.0f);

        ImVec4* Colors = Style.Colors;
        Colors[ImGuiCol_WindowBg]       = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
        Colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.40f, 0.70f, 0.40f);
        Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
        Colors[ImGuiCol_HeaderActive]   = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.12f, 0.22f, 0.40f, 1.00f);
        Colors[ImGuiCol_FrameBg]        = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
        Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.26f, 0.32f, 1.00f);
        Colors[ImGuiCol_Button]         = ImVec4(0.20f, 0.45f, 0.80f, 0.60f);
        Colors[ImGuiCol_ButtonHovered]  = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        Colors[ImGuiCol_Tab]            = ImVec4(0.12f, 0.20f, 0.35f, 0.90f);
        Colors[ImGuiCol_TabActive]      = ImVec4(0.20f, 0.40f, 0.70f, 1.00f);

        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    void Shutdown()
    {
        if (m_Window)
        {
            WorldActors.clear();

            if (PhysicsScene) PhysicsScene.reset();
            if (PhysicsSystem) { PhysicsSystem->Shutdown(); PhysicsSystem.reset(); }

            if (FBO)          glDeleteFramebuffers(1,  &FBO);
            if (TextureColor) glDeleteTextures(1,       &TextureColor);
            if (RBO)          glDeleteRenderbuffers(1,  &RBO);

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            glfwDestroyWindow(m_Window);
            glfwTerminate();
            m_Window = nullptr;
        }
    }

    void CreateFBO(int w, int h)
    {
        if (FBO)          { glDeleteFramebuffers(1,  &FBO);          FBO = 0; }
        if (TextureColor) { glDeleteTextures(1,       &TextureColor); TextureColor = 0; }
        if (RBO)          { glDeleteRenderbuffers(1,  &RBO);          RBO = 0; }

        TexWidth  = w;
        TexHeight = h;

        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenTextures(1, &TextureColor);
        glBindTexture(GL_TEXTURE_2D, TextureColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TextureColor, 0);

        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "[ERROR] Framebuffer incomplete!\n";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void BeginFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
    }

    void RenderWorld(int viewW, int viewH)
    {
        float aspect   = (viewH > 0) ? (float)viewW / (float)viewH : 1.0f;
        LastProjection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);
        LastView       = Cam.GetViewMatrix();

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, viewW, viewH);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.10f, 0.13f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (Shader)
        {
            Shader->Use();
            Shader->SetMat4("projection", LastProjection);
            Shader->SetMat4("view", LastView);

            for (int i = 0; i < (int)WorldActors.size(); ++i)
            {
                auto& Actor = WorldActors[i];
                if (!Actor) continue;

                bool isFloor    = (Actor->GetName() == "Floor");
                bool isSelected = (i == SelectedActorIndex);

                glm::vec3 color = isFloor
                    ? glm::vec3(0.35f, 0.42f, 0.35f)
                    : glm::vec3(0.40f, 0.65f, 0.95f);

                Shader->SetVec3("objectColor", color);
                Shader->SetBool("bSelected", isSelected);

                for (auto& Comp : Actor->GetComponents())
                {
                    if (auto PC = Cast<UPrimitiveComponent>(Comp))
                    {

                        FMatrix M = MakeTransformMatrix(Actor->GetTransform());
                        Shader->SetMat4("model", M);
                        PC->Render();
                    }
                }
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderUI(float DeltaTime)
    {

        RenderWorldSettingsPanel();

        WorldOutliner->RenderUI(
            WorldActors,
            SelectedActorIndex,
            [this](const std::string& ActorType) {
                if (ActorType == "Cube")
                {
                    SpawnCube("Cube_" + std::to_string(CubeCount++),
                              FVector(0.0f, 3.0f, 0.0f));
                }
                else if (ActorType == "Floor")
                {
                    auto Floor = MakeShared<AFloorActor>();
                    Floor->SetName("Floor_" + std::to_string(CubeCount++));
                    Floor->SetTransform(FTransform(FVector(0,0,0), FRotator(0,0,0), FVector(1,1,1)));
                    WorldActors.push_back(Floor);
                    Floor->BeginPlay();
                }
                else if (ActorType == "Empty")
                {
                    auto EmptyActor = MakeShared<AActor>();
                    EmptyActor->SetName("Empty_" + std::to_string(CubeCount++));
                    WorldActors.push_back(EmptyActor);
                    EmptyActor->BeginPlay();
                }
            });

        DetailsPanel->RenderUI(WorldActors, SelectedActorIndex,
            [this](TSharedPtr<AActor> Actor, bool bEnable) {
                if (bEnable)
                {
                    auto RB = MakeShared<URigidBodyComponent>();
                    RB->SetName("RigidBody");

                    Actor->SetRootComponent(RB.get());
                    Actor->AddComponent(RB);

                    RB->SetHalfExtents(FVector(0.5f));
                    RB->RegisterWithPhysicsScene(PhysicsScene.get(), Actor->GetTransform());
                    RB->BeginPlay();
                }
                else
                {

                    for (auto& Comp : Actor->GetComponents())
                    {
                        if (auto RB = dynamic_cast<URigidBodyComponent*>(Comp.get()))
                        {
                            Actor->RemoveComponent(RB);
                            break;
                        }
                    }
                }
            });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("3D Viewport");

        ImVec2 vpSize = ImGui::GetContentRegionAvail();
        int viewW = (int)vpSize.x;
        int viewH = (int)vpSize.y;

        if (viewW > 0 && viewH > 0 && (viewW != TexWidth || viewH != TexHeight))
            CreateFBO(viewW, viewH);

        if (viewW > 0 && viewH > 0)
            RenderWorld(viewW, viewH);

        ImGui::Image((void*)(intptr_t)TextureColor,
                     ImVec2((float)TexWidth, (float)TexHeight),
                     ImVec2(0, 1), ImVec2(1, 0));

        ImVec2 vpOrigin    = ImGui::GetItemRectMin();
        bool   vpHovered   = ImGui::IsItemHovered();
        ImVec2 mouseScreen = ImGui::GetMousePos();
        float  lmx = mouseScreen.x - vpOrigin.x;
        float  lmy = mouseScreen.y - vpOrigin.y;

        bool rmbDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        if (vpHovered && rmbDown)
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            Cam.Yaw   += delta.x * Cam.Sensitivity;
            Cam.Pitch -= delta.y * Cam.Sensitivity;
            Cam.Pitch  = glm::clamp(Cam.Pitch, -89.0f, 89.0f);
            Cam.UpdateVectors();

            float speed = Cam.MoveSpeed * DeltaTime;
            if (ImGui::IsKeyDown(ImGuiKey_W)) Cam.Position += Cam.Front   * speed;
            if (ImGui::IsKeyDown(ImGuiKey_S)) Cam.Position -= Cam.Front   * speed;
            if (ImGui::IsKeyDown(ImGuiKey_A)) Cam.Position -= Cam.Right   * speed;
            if (ImGui::IsKeyDown(ImGuiKey_D)) Cam.Position += Cam.Right   * speed;
            if (ImGui::IsKeyDown(ImGuiKey_E)) Cam.Position += Cam.WorldUp * speed;
            if (ImGui::IsKeyDown(ImGuiKey_Q)) Cam.Position -= Cam.WorldUp * speed;
        }

        bool lmbClicked  = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool lmbReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

        if (vpHovered && lmbClicked && viewW > 0 && viewH > 0)
            TrySelectActorAtMouse(lmx, lmy, viewW, viewH);

        bool isDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f);
        if (isDragging && vpHovered && SelectedActorIndex >= 0 && !rmbDown)
        {
            if (!bIsDraggingActor)
                BeginActorDrag(lmx, lmy, viewW, viewH);
            else
                UpdateActorDrag(lmx, lmy, viewW, viewH);
        }
        if (lmbReleased)
            bIsDraggingActor = false;

        RenderPhysicsStatusOverlay(vpOrigin, vpSize);

        ImGui::End();
        ImGui::PopStyleVar();

        int fw, fh;
        glfwGetFramebufferSize(m_Window, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void RenderPhysicsStatusOverlay(ImVec2 vpOrigin, ImVec2 vpSize)
    {
        ImVec2 OverlayPos = ImVec2(
            vpOrigin.x + vpSize.x - 200.0f,
            vpOrigin.y + vpSize.y - 60.0f);
        ImGui::SetNextWindowPos(OverlayPos, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.55f);
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowSize(ImVec2(195.0f, 50.0f), ImGuiCond_Always);
        if (ImGui::Begin("##PhysOverlay", nullptr, flags))
        {
            FVector Gravity = PhysicsScene->GetGravity();
            int PhysActorCount = 0;
            for (auto& A : WorldActors)
                if (A) for (auto& C : A->GetComponents())
                    if (Cast<URigidBodyComponent>(C)) ++PhysActorCount;

            const char* BackendName = (CurrentBackend == EPhysicsBackend::AVBD) ? "AVBD" :
                                      (CurrentBackend == EPhysicsBackend::XPBD) ? "XPBD" :
                                      (CurrentBackend == EPhysicsBackend::Classical) ? "Classical" : "Null";

            ImVec4 BackendColor = (CurrentBackend == EPhysicsBackend::AVBD) ? ImVec4(1.0f, 0.85f, 0.3f, 1.0f) :
                                  (CurrentBackend == EPhysicsBackend::XPBD) ? ImVec4(0.4f, 0.8f, 1.0f, 1.0f) :
                                  (CurrentBackend == EPhysicsBackend::Classical) ? ImVec4(0.8f, 0.4f, 0.8f, 1.0f) :
                                  ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
            ImGui::TextColored(BackendColor, "Physics: %s Backend", BackendName);
            ImGui::Text("Bodies: %d  G: %.1f", PhysActorCount, Gravity.y);
        }
        ImGui::End();
    }

    void RenderWorldSettingsPanel()
    {
        ImGui::Begin("World Settings");

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.3f, 1.0f));
        ImGui::Text("Physics Backend");
        ImGui::PopStyleColor();
        ImGui::Separator();

        bool bIsNull = (CurrentBackend == EPhysicsBackend::Null);
        bool bIsClassical = (CurrentBackend == EPhysicsBackend::Classical);
        bool bIsXPBD = (CurrentBackend == EPhysicsBackend::XPBD);
        bool bIsAVBD = (CurrentBackend == EPhysicsBackend::AVBD);

        if (bIsNull) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.2f, 0.9f));
        if (ImGui::Button("  Null  ")) RequestSwitchPhysicsBackend(EPhysicsBackend::Null);
        if (bIsNull) ImGui::PopStyleColor();

        ImGui::SameLine();

        if (bIsClassical) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.6f, 0.9f));
        if (ImGui::Button(" Classical ")) RequestSwitchPhysicsBackend(EPhysicsBackend::Classical);
        if (bIsClassical) ImGui::PopStyleColor();

        ImGui::SameLine();

        if (bIsXPBD) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.9f));
        if (ImGui::Button("  XPBD  ")) RequestSwitchPhysicsBackend(EPhysicsBackend::XPBD);
        if (bIsXPBD) ImGui::PopStyleColor();

        ImGui::SameLine();

        if (bIsAVBD) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.4f, 0.1f, 0.9f));
        if (ImGui::Button("  AVBD  ")) RequestSwitchPhysicsBackend(EPhysicsBackend::AVBD);
        if (bIsAVBD) ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Gravity");
        ImGui::PushItemWidth(150.0f);
        if (ImGui::DragFloat("Y##Gravity", &WorldGravityY, 0.1f, -50.0f, 0.0f, "%.2f m/s2"))
            PhysicsScene->SetGravity(FVector(0.0f, WorldGravityY, 0.0f));
        ImGui::PopItemWidth();

        if (CurrentBackend == EPhysicsBackend::AVBD)
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.35f, 0.1f, 0.5f));
            bool open = ImGui::CollapsingHeader("AVBD Solver Options", ImGuiTreeNodeFlags_DefaultOpen);
            ImGui::PopStyleColor();
            if (open)
            {
                if (AVBDPhysicsScene* AVBDScene = dynamic_cast<AVBDPhysicsScene*>(PhysicsScene.get()))
                {
                    FAVBDConfig& Cfg = AVBDScene->Config;
                    ImGui::PushItemWidth(120.0f);
                    ImGui::DragInt  ("Iterations", &Cfg.SolverIterations, 1, 1, 64);
                    ImGui::DragFloat("Alpha (Stab)",  &Cfg.Alpha, 0.01f, 0.0f, 1.0f, "%.3f");
                    ImGui::DragFloat("Beta Lin",      &Cfg.BetaLin, 10.0f, 0.0f, 100000.0f, "%.1f");
                    ImGui::DragFloat("Beta Ang",      &Cfg.BetaAng, 1.0f, 0.0f, 10000.0f, "%.1f");
                    ImGui::DragFloat("Gamma (Warm)",  &Cfg.Gamma, 0.001f, 0.0f, 1.0f, "%.4f");
                    ImGui::DragFloat("Friction", &Cfg.DefaultFriction, 0.01f, 0.0f, 2.0f, "%.3f");
                    ImGui::PopItemWidth();
                }
            }
        }

        else if (CurrentBackend == EPhysicsBackend::XPBD)
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.35f, 0.5f, 0.5f));
            bool open = ImGui::CollapsingHeader("XPBD Solver Options", ImGuiTreeNodeFlags_DefaultOpen);
            ImGui::PopStyleColor();
            if (open)
            {
                if (XPBDPhysicsScene* XPBDScene = dynamic_cast<XPBDPhysicsScene*>(PhysicsScene.get()))
                {
                    FXPBDConfig& Cfg = XPBDScene->Config;
                    ImGui::PushItemWidth(120.0f);
                    ImGui::DragInt  ("Iterations", &Cfg.SolverIterations, 1, 1, 64);
                    ImGui::DragFloat("Compliance", &Cfg.Compliance, 0.0001f, 0.0f, 0.1f, "%.5f");
                    ImGui::DragFloat("Restitution", &Cfg.Restitution, 0.01f, 0.0f, 1.0f, "%.3f");
                    ImGui::DragFloat("Friction", &Cfg.Friction, 0.01f, 0.0f, 2.0f, "%.3f");
                    ImGui::PopItemWidth();
                }
            }
        }

        ImGui::End();
    }

    void EndFrame()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }
        glfwSwapBuffers(m_Window);
    }

    int         m_Width, m_Height;
    std::string m_Title;
    GLFWwindow* m_Window;
    float       LastFrameTime = 0.0f;
    TSharedPtr<UShader> Shader;

    std::vector<TSharedPtr<AActor>> WorldActors;
    int SelectedActorIndex = -1;
    int CubeCount          = 1;

    FCamera Cam;
    glm::mat4 LastProjection = glm::mat4(1.0f);
    glm::mat4 LastView       = glm::mat4(1.0f);

    std::unique_ptr<IPhysicsSystem> PhysicsSystem;
    std::unique_ptr<IPhysicsScene>  PhysicsScene;
    float           PhysicsAccumulator  = 0.0f;
    EPhysicsBackend CurrentBackend      = EPhysicsBackend::Null;
    float           WorldGravityY       = -9.81f;

    bool            bPendingBackendSwitch = false;
    EPhysicsBackend PendingBackend        = EPhysicsBackend::Null;

    std::unique_ptr<UWorldOutliner> WorldOutliner;
    std::unique_ptr<UDetailsPanel>  DetailsPanel;

    bool      bIsDraggingActor = false;
    float     DragPlaneY       = 0.0f;
    glm::vec3 DragHitOffset    = glm::vec3(0.0f);

    unsigned int FBO, TextureColor, RBO;
    int TexWidth, TexHeight;
};

int main()
{
    UApplication app(1600, 900, "Custom Engine — Physics Edition");
    if (!app.Init())
    {
        std::cerr << "Engine Initialization Failed\n";
        return -1;
    }
    app.Run();
    return 0;
}
