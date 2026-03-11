#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Custom Engine Headers (유저 분의 프로젝트에 있는 파일들)
#include "CoreMinimal.h"
#include "UObject.h"
#include "UActorComponent.h"
#include "USceneComponent.h"
#include "UPrimitiveComponent.h"
#include "UStaticMeshComponent.h"
#include "AActor.h"
#include "ACubeActor.h"

// ------------------------------------------------------------------------
// [셰이더 소스] (수정 없음 - 조명 연산 포함)
// ------------------------------------------------------------------------
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

// ------------------------------------------------------------------------
// [Application 클래스]
// ------------------------------------------------------------------------
class Application {
public:
    Application(int width, int height, const std::string& title)
        : m_Width(width), m_Height(height), m_Title(title), m_Window(nullptr), ShaderProgram(0), FBO(0), TextureColor(0), RBO(0) {
    }

    ~Application() { Shutdown(); }

    bool Init() {
        if (!InitGLFW()) return false;
        if (!InitGLAD()) return false;
        InitImGui();

        // 컴파일 셰이더
        CompileShaders();

        // --- [추가됨] FBO 및 텍스처 설정 ---
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenTextures(1, &TextureColor);
        glBindTexture(GL_TEXTURE_2D, TextureColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TexWidth, TexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TextureColor, 0);

        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, TexWidth, TexHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "[ERROR] Framebuffer is not complete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // -----------------------------------

        TSharedPtr<ACubeActor> MyCube = MakeShared<ACubeActor>();
        MyCube->SetName("PlayerCube");

        // Set initial transform
        FTransform InitialTransform(FVector(0, 0, -2.0f), FRotator(0, 0, 0), FVector(1, 1, 1));
        MyCube->SetTransform(InitialTransform);

        WorldActors.push_back(MyCube);

        // 2. Trigger BeginPlay for all actors
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
            float CurrentTime = static_cast<float>(glfwGetTime());
            float DeltaTime = CurrentTime - LastFrameTime;
            LastFrameTime = CurrentTime;

            // 1. Event Polling
            glfwPollEvents();

            // 2. Engine Logic Tick (World Tick -> Actor Tick -> Component Tick)
            for (auto& Actor : WorldActors) {
                if (Actor) Actor->Tick(DeltaTime);
            }

            // --- [수정됨] 렌더링 파이프라인 분리 ---

            // 3. Main 3D World Rendering (FBO에 렌더링)
            RenderWorld();

            // 4. 화면을 지우고 기본 프레임버퍼 준비
            int display_w, display_h;
            glfwGetFramebufferSize(m_Window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glDisable(GL_DEPTH_TEST); // UI 렌더링을 위해 Depth 테스트 끄기

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // 바깥 전체 배경색 (검은색)
            glClear(GL_COLOR_BUFFER_BIT);

            // 5. UI Rendering (FBO 텍스처를 ImGui 창 안에 렌더링)
            BeginFrame();
            RenderUI();

            // 6. Finalize frame
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
        glfwSwapInterval(1); // Enable V-Sync
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
        int success;
        char infoLog[512];

        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "[ERROR] Vertex Shader Compilation Failed:\n" << infoLog << std::endl;
        }

        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "[ERROR] Fragment Shader Compilation Failed:\n" << infoLog << std::endl;
        }

        ShaderProgram = glCreateProgram();
        glAttachShader(ShaderProgram, vertexShader);
        glAttachShader(ShaderProgram, fragmentShader);
        glLinkProgram(ShaderProgram);
        glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ShaderProgram, 512, NULL, infoLog);
            std::cerr << "[ERROR] Shader Program Linking Failed:\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void Shutdown() {
        if (m_Window) {
            WorldActors.clear();

            // --- [추가됨] FBO 자원 해제 ---
            if (FBO) glDeleteFramebuffers(1, &FBO);
            if (TextureColor) glDeleteTextures(1, &TextureColor);
            if (RBO) glDeleteRenderbuffers(1, &RBO);
            // ------------------------------

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

    // --- [수정됨] 3D 월드 렌더링을 FBO에 하도록 변경 ---
    void RenderWorld() {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, TexWidth, TexHeight);
        glEnable(GL_DEPTH_TEST);

        // 3D 공간의 배경색 (ImGui 내부 화면 창 색상)
        glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(ShaderProgram);

        float projection[16] = {
            1.732f,  0.0f,    0.0f,    0.0f,
            0.0f,    3.078f,  0.0f,    0.0f,
            0.0f,    0.0f,   -1.002f, -1.0f,
            0.0f,    0.0f,   -0.2002f, 0.0f
        };

        float view[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f,-5.0f, 1.0f
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

        // FBO 렌더링 끝, 기본 프레임버퍼로 복구
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // --- [수정됨] 3D 화면을 출력할 뷰포트 창 추가 ---
    void RenderUI() {
        // 1. 기존 아웃라이너
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

        // 2. 3D 화면 뷰포트
        ImGui::Begin("3D Viewport");
        ImVec2 winSize = ImGui::GetContentRegionAvail();

        // FBO 텍스처를 ImGui 이미지로 렌더링 (uv 좌표 반전으로 상하 뒤집힘 해결)
        ImGui::Image((void*)(intptr_t)TextureColor, winSize, ImVec2(0, 1), ImVec2(1, 0));

        ImGui::End();
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

    // --- [추가됨] FBO 멤버 변수 ---
    unsigned int FBO, TextureColor, RBO;
    int TexWidth = 1280; // 렌더링 해상도
    int TexHeight = 720;
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