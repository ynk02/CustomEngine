#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>

// ImGui
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

// Simple Shader Sources for the example
const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "out vec3 Normal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "   Normal = aNormal;\n"
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 Normal;\n"
    "void main()\n"
    "{\n"
    "   vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));\n"
    "   float diff = max(dot(Normal, lightDir), 0.0);\n"
    "   vec3 color = vec3(0.5, 0.7, 1.0) * (diff + 0.3); // Simple lighting\n"
    "   FragColor = vec4(color, 1.0);\n"
    "}\n\0";

// Simple Math Helper has been moved to CoreMinimal.h

class Application {
public:
    Application(int width, int height, const std::string& title)
        : m_Width(width), m_Height(height), m_Title(title), m_Window(nullptr), ShaderProgram(0) {}

    ~Application() { Shutdown(); }

    bool Init() {
        if (!InitGLFW()) return false;
        if (!InitGLAD()) return false;
        InitImGui();

        // Enable Depth Testing for 3D
        glEnable(GL_DEPTH_TEST);

        // Compile Shaders
        CompileShaders();

        // --- UE5 Engine Architecture Initialization ---
        // 1. Spawn Actors into the "World"
        TSharedPtr<ACubeActor> MyCube = MakeShared<ACubeActor>();
        MyCube->SetName("PlayerCube");
        
        // Set initial transform
        FTransform InitialTransform(FVector(0, 0, -2.0f), FRotator(0, 0, 0), FVector(1, 1, 1));
        MyCube->SetTransform(InitialTransform);
        
        WorldActors.push_back(MyCube);

        // 2. Trigger BeginPlay for all actors (similar to UWorld::BeginPlay)
        float CurrentTime = glfwGetTime();
        LastFrameTime = CurrentTime;

        for (auto& Actor : WorldActors) {
            if (Actor) Actor->BeginPlay();
        }

        return true;
    }

    void Run() {
        while (!glfwWindowShouldClose(m_Window)) {
            // Calculate DeltaTime
            float CurrentTime = glfwGetTime();
            float DeltaTime = CurrentTime - LastFrameTime;
            LastFrameTime = CurrentTime;

            // 1. Event Polling
            glfwPollEvents();
            
            // 2. Engine Logic Tick (World Tick -> Actor Tick -> Component Tick)
            for (auto& Actor : WorldActors) {
                if (Actor) Actor->Tick(DeltaTime);
            }

            // 3. UI Begin
            BeginFrame();
            RenderUI();

            // 4. Main Rendering (World Render Loop)
            RenderWorld();

            // 5. Swap & UI End
            EndFrame();
        }
    }

private:
    bool InitGLFW() {
        if (!glfwInit()) return false;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), NULL, NULL);
        if (!m_Window) { glfwTerminate(); return false; }
        glfwMakeContextCurrent(m_Window);
        return true;
    }

    bool InitGLAD() {
        return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) != 0;
    }

    void InitImGui() {
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

    void CompileShaders() {
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        ShaderProgram = glCreateProgram();
        glAttachShader(ShaderProgram, vertexShader);
        glAttachShader(ShaderProgram, fragmentShader);
        glLinkProgram(ShaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void Shutdown() {
        if (m_Window) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            glfwDestroyWindow(m_Window);
            glfwTerminate();
            m_Window = nullptr;
        }
    }

    void BeginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
    }

    void RenderUI() {
        ImGui::Begin("World Outliner");
        for (auto& Actor : WorldActors) {
            if (Actor) {
                if (ImGui::TreeNode(Actor->GetName().c_str())) {
                    FTransform t = Actor->GetTransform();
                    ImGui::Text("Loc: %.2f %.2f %.2f", t.Location.X, t.Location.Y, t.Location.Z);
                    ImGui::Text("Rot: %.2f %.2f %.2f", t.Rotation.Pitch, t.Rotation.Yaw, t.Rotation.Roll);
                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }

    void RenderWorld() {
        glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(ShaderProgram);

        // Simple perspective projection & view matrices (Hardcoded for example)
        float projection[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.77f, 0.0f, 0.0f, // roughly 16:9 aspect ratio
            0.0f, 0.0f, -1.02f, -1.0f,
            0.0f, 0.0f, -0.2f, 0.0f
        };
        float view[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, -5.0f, 1.0f // move camera back 5 units
        };

        glUniformMatrix4fv(glGetUniformLocation(ShaderProgram, "projection"), 1, GL_FALSE, projection);
        glUniformMatrix4fv(glGetUniformLocation(ShaderProgram, "view"), 1, GL_FALSE, view);

        for (auto& Actor : WorldActors) {
            if (!Actor) continue;

            const auto& Components = Actor->GetComponents();
            for (auto& Comp : Components) {

                if (auto PrimitiveComp = Cast<UPrimitiveComponent>(Comp)) {
                    

                    FTransform CompTransform = PrimitiveComp->GetTransform();
                    FMatrix ModelMatrix = MakeTransformMatrix(CompTransform);
                    glUniformMatrix4fv(glGetUniformLocation(ShaderProgram, "model"), 1, GL_FALSE, ModelMatrix.GetPtr());


                    PrimitiveComp->Render();
                }
            }
        }
    }

    void EndFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }

        glfwSwapBuffers(m_Window);
    }

private:
    int m_Width, m_Height;
    std::string m_Title;
    GLFWwindow* m_Window;
    float LastFrameTime = 0.0f;
    unsigned int ShaderProgram;
    
    // Engine "World" state
    std::vector<TSharedPtr<AActor>> WorldActors;
};

int main() {
    Application app(1280, 720, "Custom Engine");
    if (!app.Init()) {
        std::cerr << "Engine Initialization Failed" << std::endl;
        return -1;
    }
    app.Run();
    return 0;
}