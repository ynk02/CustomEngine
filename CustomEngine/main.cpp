#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cfloat>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Custom Engine Headers
#include "CoreMinimal.h"
#include "UObject.h"
#include "UActorComponent.h"
#include "USceneComponent.h"
#include "UPrimitiveComponent.h"
#include "UStaticMeshComponent.h"
#include "AActor.h"
#include "ACubeActor.h"
#include "AFloorActor.h"

#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"out vec3 Normal;\n"
"out vec3 FragPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"   FragPos = vec3(model * vec4(aPos, 1.0));\n"
"   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 Normal;\n"
"in vec3 FragPos;\n"
"uniform vec3 objectColor;\n"
"uniform bool bSelected;\n"
"void main()\n"
"{\n"
"   vec3 lightDir = normalize(vec3(1.0, 2.0, 1.0));\n"
"   float diff = max(dot(normalize(Normal), lightDir), 0.0);\n"
"   vec3 ambient = 0.25 * objectColor;\n"
"   vec3 diffuse = diff * objectColor;\n"
"   vec3 col = ambient + diffuse;\n"
"   if (bSelected) col = mix(col, vec3(1.0, 0.8, 0.2), 0.35);\n"
"   FragColor = vec4(col, 1.0);\n"
"}\n\0";

// -----------------------------------------------------------------------
// Ray
// -----------------------------------------------------------------------
struct FRay {
    glm::vec3 Origin;
    glm::vec3 Direction;
};

// -----------------------------------------------------------------------
// Camera
// -----------------------------------------------------------------------
struct FCamera {
    glm::vec3 Position  = { 0.0f, 3.0f, 10.0f };
    float     Yaw       = -90.0f;
    float     Pitch     = -15.0f;
    float     MoveSpeed = 5.0f;
    float     Sensitivity = 0.15f;

    glm::vec3 Front  = { 0.0f, 0.0f, -1.0f };
    glm::vec3 Right  = { 1.0f, 0.0f,  0.0f };
    glm::vec3 WorldUp= { 0.0f, 1.0f,  0.0f };

    void UpdateVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
    }

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, WorldUp);
    }
};

// -----------------------------------------------------------------------
// Application
// -----------------------------------------------------------------------
class Application {
public:
    Application(int width, int height, const std::string& title)
        : m_Width(width), m_Height(height), m_Title(title),
          m_Window(nullptr), ShaderProgram(0),
          FBO(0), TextureColor(0), RBO(0), TexWidth(1), TexHeight(1)
    {}

    ~Application() { Shutdown(); }

    bool Init()
    {
        if (!InitGLFW()) return false;
        if (!InitGLAD()) return false;
        InitImGui();
        CompileShaders();
        CreateFBO(m_Width, m_Height);

        // Store app pointer on GLFW window for callbacks
        glfwSetWindowUserPointer(m_Window, this);

        // ---- Live resize during window drag (Windows blocks PollEvents) ----
        glfwSetWindowRefreshCallback(m_Window, [](GLFWwindow* w) {
            auto* app = static_cast<Application*>(glfwGetWindowUserPointer(w));
            if (app) app->RenderOneFrame(0.016f);
        });

        // ---- Default floor plane ----
        TSharedPtr<AFloorActor> Floor = MakeShared<AFloorActor>();
        Floor->SetName("Floor");
        Floor->SetTransform(FTransform(FVector(0,0,0), FRotator(0,0,0), FVector(1,1,1)));
        WorldActors.push_back(Floor);

        // ---- Default cube ----
        SpawnCube("Cube_0", FVector(0.0f, 0.5f, 0.0f));

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
            float dt  = now - LastFrameTime;
            LastFrameTime = now;

            glfwPollEvents();

            for (auto& Actor : WorldActors)
                if (Actor) Actor->Tick(dt);

            RenderOneFrame(dt);
        }
    }

    // Public: called from the window-refresh callback during resize
    void RenderOneFrame(float dt)
    {
        BeginFrame();
        RenderUI(dt);
        EndFrame();
    }

private:
    // ----------------------------------------------------------------
    // FBO
    // ----------------------------------------------------------------
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

    // ----------------------------------------------------------------
    // Spawn
    // ----------------------------------------------------------------
    void SpawnCube(const std::string& Name, const FVector& Location)
    {
        auto Cube = MakeShared<ACubeActor>();
        Cube->SetName(Name);
        Cube->SetTransform(FTransform(Location, FRotator(0,0,0), FVector(1,1,1)));
        WorldActors.push_back(Cube);
        Cube->BeginPlay();
    }

    // ----------------------------------------------------------------
    // Ray helpers
    // ----------------------------------------------------------------
    FRay ComputeRayFromMouse(float mouseX, float mouseY, int viewW, int viewH) const
    {
        float nx =  (mouseX / viewW)  * 2.0f - 1.0f;
        float ny = -(mouseY / viewH)  * 2.0f + 1.0f;

        glm::vec4 rayClip = glm::vec4(nx, ny, -1.0f, 1.0f);
        glm::vec4 rayEye  = glm::inverse(LastProjection) * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
        glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(LastView) * rayEye));

        return { Cam.Position, rayWorld };
    }

    static bool RayAABBIntersect(const FRay& ray,
                                  const glm::vec3& aabbMin,
                                  const glm::vec3& aabbMax,
                                  float& tOut)
    {
        float tMin = -FLT_MAX, tMax = FLT_MAX;
        for (int i = 0; i < 3; ++i)
        {
            if (std::abs(ray.Direction[i]) < 1e-6f)
            {
                if (ray.Origin[i] < aabbMin[i] || ray.Origin[i] > aabbMax[i]) return false;
            }
            else
            {
                float t1 = (aabbMin[i] - ray.Origin[i]) / ray.Direction[i];
                float t2 = (aabbMax[i] - ray.Origin[i]) / ray.Direction[i];
                if (t1 > t2) std::swap(t1, t2);
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                if (tMin > tMax) return false;
            }
        }
        tOut = (tMin > 0.0f) ? tMin : tMax;
        return tOut > 0.0f;
    }

    void TrySelectActorAtMouse(float mouseX, float mouseY, int viewW, int viewH)
    {
        FRay ray = ComputeRayFromMouse(mouseX, mouseY, viewW, viewH);

        float closestT   = FLT_MAX;
        int   closestIdx = -1;

        for (int i = 0; i < (int)WorldActors.size(); ++i)
        {
            auto& actor = WorldActors[i];
            if (!actor || actor->GetName() == "Floor") continue;

            FTransform t  = actor->GetTransform();
            glm::vec3 half = t.Scale3D * 0.5f;
            glm::vec3 mn   = t.Location - half;
            glm::vec3 mx   = t.Location + half;

            float hitT;
            if (RayAABBIntersect(ray, mn, mx, hitT) && hitT < closestT)
            {
                closestT   = hitT;
                closestIdx = i;
            }
        }

        SelectedActorIndex = closestIdx;
    }

    // Begin drag: record the offset of the mouse-hit from the actor's origin on XZ plane
    void BeginActorDrag(float mouseX, float mouseY, int viewW, int viewH)
    {
        if (SelectedActorIndex < 0 || SelectedActorIndex >= (int)WorldActors.size()) return;
        auto& actor = WorldActors[SelectedActorIndex];
        if (!actor) return;

        FRay ray     = ComputeRayFromMouse(mouseX, mouseY, viewW, viewH);
        DragPlaneY   = actor->GetTransform().Location.y;

        if (std::abs(ray.Direction.y) > 1e-6f)
        {
            float t = (DragPlaneY - ray.Origin.y) / ray.Direction.y;
            if (t > 0.0f)
            {
                glm::vec3 hitPt = ray.Origin + t * ray.Direction;
                DragHitOffset = hitPt - actor->GetTransform().Location;
                bIsDraggingActor = true;
            }
        }
    }

    void UpdateActorDrag(float mouseX, float mouseY, int viewW, int viewH)
    {
        if (!bIsDraggingActor || SelectedActorIndex < 0) return;
        auto& actor = WorldActors[SelectedActorIndex];
        if (!actor) return;

        FRay ray = ComputeRayFromMouse(mouseX, mouseY, viewW, viewH);

        if (std::abs(ray.Direction.y) > 1e-6f)
        {
            float t = (DragPlaneY - ray.Origin.y) / ray.Direction.y;
            if (t > 0.0f)
            {
                glm::vec3 hitPt  = ray.Origin + t * ray.Direction;
                glm::vec3 newLoc = hitPt - DragHitOffset;
                newLoc.y         = DragPlaneY; // keep Y fixed

                FTransform trans = actor->GetTransform();
                trans.Location   = newLoc;
                actor->SetTransform(trans);
            }
        }
    }

    // ----------------------------------------------------------------
    // Init helpers
    // ----------------------------------------------------------------
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
        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    void CompileShaders()
    {
        int  success;
        char infoLog[512];

        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertexShaderSource, NULL);
        glCompileShader(vs);
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) { glGetShaderInfoLog(vs, 512, NULL, infoLog); std::cerr << "[ERROR] VS: " << infoLog << "\n"; }

        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragmentShaderSource, NULL);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) { glGetShaderInfoLog(fs, 512, NULL, infoLog); std::cerr << "[ERROR] FS: " << infoLog << "\n"; }

        ShaderProgram = glCreateProgram();
        glAttachShader(ShaderProgram, vs);
        glAttachShader(ShaderProgram, fs);
        glLinkProgram(ShaderProgram);
        glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);
        if (!success) { glGetProgramInfoLog(ShaderProgram, 512, NULL, infoLog); std::cerr << "[ERROR] Link: " << infoLog << "\n"; }

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void Shutdown()
    {
        if (m_Window)
        {
            WorldActors.clear();
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

    // ----------------------------------------------------------------
    // Frame
    // ----------------------------------------------------------------
    void BeginFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
    }

    void RenderWorld(int viewW, int viewH)
    {
        float aspect = (viewH > 0) ? (float)viewW / (float)viewH : 1.0f;
        LastProjection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);
        LastView       = Cam.GetViewMatrix();

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, viewW, viewH);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.12f, 0.18f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(ShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(ShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(LastProjection));
        glUniformMatrix4fv(glGetUniformLocation(ShaderProgram, "view"),       1, GL_FALSE, glm::value_ptr(LastView));

        for (int i = 0; i < (int)WorldActors.size(); ++i)
        {
            auto& Actor = WorldActors[i];
            if (!Actor) continue;

            bool isFloor    = (Actor->GetName() == "Floor");
            bool isSelected = (i == SelectedActorIndex);

            glm::vec3 color = isFloor ? glm::vec3(0.45f, 0.50f, 0.45f)
                                      : glm::vec3(0.45f, 0.65f, 0.90f);
            glUniform3fv(glGetUniformLocation(ShaderProgram, "objectColor"), 1, glm::value_ptr(color));
            glUniform1i(glGetUniformLocation(ShaderProgram, "bSelected"), isSelected ? 1 : 0);

            for (auto& Comp : Actor->GetComponents())
            {
                if (auto PC = Cast<UPrimitiveComponent>(Comp))
                {
                    FMatrix M = MakeTransformMatrix(PC->GetTransform());
                    glUniformMatrix4fv(glGetUniformLocation(ShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(M));
                    PC->Render();
                }
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderUI(float DeltaTime)
    {
        // ---- World Outliner ----
        ImGui::Begin("World Outliner");
        if (ImGui::Button("+ Add Cube"))
        {
            SpawnCube("Cube_" + std::to_string(CubeCount++), FVector(0.0f, 0.5f, 0.0f));
        }
        ImGui::Separator();
        for (int i = 0; i < (int)WorldActors.size(); ++i)
        {
            auto& Actor = WorldActors[i];
            if (!Actor) continue;
            bool sel = (SelectedActorIndex == i);
            if (ImGui::Selectable(Actor->GetName().c_str(), sel))
                SelectedActorIndex = i;
        }
        ImGui::End();

        // ---- Details ----
        ImGui::Begin("Details");
        if (SelectedActorIndex >= 0 && SelectedActorIndex < (int)WorldActors.size())
        {
            auto& Actor = WorldActors[SelectedActorIndex];
            if (Actor)
            {
                ImGui::Text("Actor: %s", Actor->GetName().c_str());
                ImGui::Separator();
                FTransform t = Actor->GetTransform();
                bool changed = false;
                changed |= ImGui::DragFloat3("Location", &t.Location.x, 0.05f);
                changed |= ImGui::DragFloat3("Rotation", &t.Rotation.x, 1.0f);
                changed |= ImGui::DragFloat3("Scale",    &t.Scale3D.x,  0.05f, 0.01f, 100.0f);
                if (changed) Actor->SetTransform(t);
            }
        }
        else
        {
            ImGui::TextDisabled("Select an actor");
        }
        ImGui::End();

        // ---- 3D Viewport ----
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("3D Viewport");

        ImVec2 vpSize = ImGui::GetContentRegionAvail();
        int viewW = (int)vpSize.x;
        int viewH = (int)vpSize.y;

        // Resize FBO to match viewport content size
        if (viewW > 0 && viewH > 0 && (viewW != TexWidth || viewH != TexHeight))
            CreateFBO(viewW, viewH);

        // Render 3D scene into FBO
        if (viewW > 0 && viewH > 0)
            RenderWorld(viewW, viewH);

        // Display FBO texture
        ImGui::Image((void*)(intptr_t)TextureColor,
                     ImVec2((float)TexWidth, (float)TexHeight),
                     ImVec2(0, 1), ImVec2(1, 0));

        // Screen position of the image (top-left corner) for local mouse coords
        ImVec2 vpOrigin = ImGui::GetItemRectMin();
        bool   vpHovered   = ImGui::IsItemHovered();
        ImVec2 mouseScreen = ImGui::GetMousePos();
        float  lmx = mouseScreen.x - vpOrigin.x; // local X within viewport image
        float  lmy = mouseScreen.y - vpOrigin.y; // local Y within viewport image

        // ---- RIGHT CLICK: camera orbit + WASD ----
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

        // ---- LEFT CLICK: select actor in viewport ----
        bool lmbClicked  = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool lmbDown     = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bool lmbReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

        // On click: ray-cast to select (only if not starting a drag immediately)
        if (vpHovered && lmbClicked && viewW > 0 && viewH > 0)
        {
            TrySelectActorAtMouse(lmx, lmy, viewW, viewH);
        }

        // ---- LEFT DRAG: move selected actor along XZ plane ----
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

        ImGui::End();
        ImGui::PopStyleVar();

        // Clear main framebuffer
        int fw, fh;
        glfwGetFramebufferSize(m_Window, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
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

private:
    int         m_Width, m_Height;
    std::string m_Title;
    GLFWwindow* m_Window;
    float       LastFrameTime = 0.0f;
    unsigned int ShaderProgram;

    std::vector<TSharedPtr<AActor>> WorldActors;
    int  SelectedActorIndex = -1;
    int  CubeCount  = 1;

    FCamera Cam;

    // Last frame's matrices (needed for ray casting in UI pass)
    glm::mat4 LastProjection = glm::mat4(1.0f);
    glm::mat4 LastView       = glm::mat4(1.0f);

    // Actor drag state
    bool      bIsDraggingActor = false;
    float     DragPlaneY       = 0.0f;
    glm::vec3 DragHitOffset    = glm::vec3(0.0f);

    unsigned int FBO, TextureColor, RBO;
    int TexWidth, TexHeight;
};

int main()
{
    Application app(1600, 900, "Custom Engine");
    if (!app.Init())
    {
        std::cerr << "Engine Initialization Failed\n";
        return -1;
    }
    app.Run();
    return 0;
}