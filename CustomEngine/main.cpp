#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

// ImGui 헤더
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


// 애플리케이션 클래스: 윈도우 생성, 초기화, 라이프사이클 관리
class Application {
public:
    Application(int width, int height, const std::string& title)
        : m_Width(width), m_Height(height), m_Title(title), m_Window(nullptr) {}

    ~Application() {
        Shutdown();
    }

    bool Init() {
        if (!InitGLFW()) return false;
        if (!InitGLAD()) return false;
        InitImGui();
        return true;
    }

    void Run() {
        while (!glfwWindowShouldClose(m_Window)) {
            // 1. 이벤트 처리
            glfwPollEvents();

            // 2. 프레임 시작
            BeginFrame();

            // 3. UI 렌더링
            RenderUI();

            // 4. 메인 렌더링 (OpenGL)
            Render();

            // 5. 프레임 종료 및 스왑
            EndFrame();
        }
    }

private:
    bool InitGLFW() {
        if (!glfwInit()) {
            std::cerr << "GLFW 초기화 실패!" << std::endl;
            return false;
        }

        // OpenGL 3.3 Core Profile 설정
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), NULL, NULL);
        if (m_Window == NULL) {
            std::cerr << "GLFW 창 생성 실패!" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(m_Window);
        glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
        
        // 콜백에서 인스턴스에 접근할 수 있도록 윈도우 유저 포인터 설정
        glfwSetWindowUserPointer(m_Window, this);

        return true;
    }

    bool InitGLAD() {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "GLAD 초기화 실패!" << std::endl;
            return false;
        }
        return true;
    }

    void InitImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 키보드 컨트롤 활성화
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // 도킹 활성화
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // 멀티 뷰포트 활성화 (독립된 창)

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
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

        // 메인 도킹 스페이스 생성
        ImGui::DockSpaceOverViewport();
    }

    void RenderUI() {
        // 테스트 데모 창
        ImGui::ShowDemoWindow();

        // 커스텀 엔진 컨트롤 패널
        ImGui::Begin("Engine Control");
        ImGui::Text("Hello, Game Engine!");
        if (ImGui::Button("Click Me")) {
            std::cout << "Button Clicked!" << std::endl;
        }
        ImGui::End();
    }

    void Render() {
        // 화면 클리어
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // TODO: 실제 세계/씬 렌더링 코드 위치
    }

    void EndFrame() {
        // ImGui 렌더링
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 멀티 뷰포트 렌더링 업데이트
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(m_Window);
    }

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
        
        // 필요한 경우 Application 인스턴스 데이터를 업데이트 가능
        // Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
        // if (app) { app->m_Width = width; app->m_Height = height; }
    }

private:
    int m_Width;
    int m_Height;
    std::string m_Title;
    GLFWwindow* m_Window;
};

int main() {
    Application app(1280, 720, "My Custom Game Engine");
    
    if (!app.Init()) {
        std::cerr << "엔진 초기화에 실패했습니다." << std::endl;
        return -1;
    }

    app.Run();

    // 애플리케이션 소멸자(~Application)에서 자원 정리(Shutdown)가 자동으로 호출됩니다.
    return 0;
}