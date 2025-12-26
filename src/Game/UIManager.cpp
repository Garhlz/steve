#include "Game/UIManager.h"
#include "Game/Game.h"

UIManager::UIManager() : showControls(false) {}

UIManager::~UIManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::Init(GLFWwindow* window) {
    // 1. 创建上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // 2. 设置风格
    ImGui::StyleColorsDark();

    // 3. 初始化绑定
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    // 放大字体
    io.FontGlobalScale = 2.0f;
}

void UIManager::Render(Game& game) {
    // 开始新的一帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 根据游戏状态分发渲染逻辑
    if (game.State == GAME_MENU) {
        RenderMainMenu(game);
    } 
    else if (game.State == GAME_PAUSED) {
        RenderPauseMenu(game);
    } 
    else if (game.State == GAME_ACTIVE) {
        RenderHUD(game);
    }

    // 渲染指令提交
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// === 辅助逻辑实现 ===

void UIManager::RenderControlsList() {
    // ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "--- CONTROLS ---");
    ImGui::Spacing();

    ImGui::Columns(2, "controls_columns", false); 
    ImGui::SetColumnWidth(0, 250); 

    ImGui::Text("W / A / S / D"); ImGui::NextColumn(); ImGui::Text(": Move Character"); ImGui::NextColumn();
    ImGui::Text("Mouse");         ImGui::NextColumn(); ImGui::Text(": Rotate Camera");  ImGui::NextColumn();
    ImGui::Text("TAB");           ImGui::NextColumn(); ImGui::Text(": Switch Mode");    ImGui::NextColumn();
    ImGui::Text("T");             ImGui::NextColumn(); ImGui::Text(": Switch Character");    ImGui::NextColumn();
    ImGui::Text("R (Hold)");      ImGui::NextColumn(); ImGui::Text(": Swing Sword");    ImGui::NextColumn();
    ImGui::Text("Q / E (Hold)");  ImGui::NextColumn(); ImGui::Text(": Shake Head");     ImGui::NextColumn();
    ImGui::Text("SPACE");         ImGui::NextColumn(); ImGui::Text(": Jump");           ImGui::NextColumn();
    ImGui::Text("B");             ImGui::NextColumn(); ImGui::Text(": Day/Night");      ImGui::NextColumn();
    ImGui::Text("ESC");           ImGui::NextColumn(); ImGui::Text(": Pause/Resume");   ImGui::NextColumn();
    
    ImGui::Columns(1);
    
    // Back 按钮推到底部
    ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 60.0f);
    if (ImGui::Button("BACK", ImVec2(-1.0f, 50.0f))) {
        showControls = false;
    }
}

void UIManager::RenderMainMenu(Game& game) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImVec2 menuSize(600, 450);

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(menuSize);

    ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    // 标题
    float fontScale = 1.5f;
    ImGui::SetWindowFontScale(fontScale);
    const char* title = "Steve's Adventure";
    float textWidth = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - textWidth) * 0.5f);
    ImGui::Text("%s", title);
    ImGui::SetWindowFontScale(1.2f);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (showControls) {
        RenderControlsList();
    } else {
        float availY = ImGui::GetContentRegionAvail().y;
        float spacing = 10.0f;
        float btnHeight = (availY - spacing * 2.0f) / 3.0f;

        if (ImGui::Button("START GAME", ImVec2(-1.0f, btnHeight))) {
            game.State = GAME_ACTIVE;
            game.SetMouseMode(true);
            showControls = false;
        }
        ImGui::Dummy(ImVec2(0.0f, spacing));
        
        if (ImGui::Button("CONTROLS", ImVec2(-1.0f, btnHeight))) {
            showControls = true;
        }
        ImGui::Dummy(ImVec2(0.0f, spacing));

        if (ImGui::Button("QUIT", ImVec2(-1.0f, btnHeight))) {
            glfwSetWindowShouldClose(game.GetWindow(), true);
        }
    }
    ImGui::End();
}

void UIManager::RenderPauseMenu(Game& game) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImVec2 menuSize(600, 450);

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(menuSize);

    ImGui::Begin("Pause Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    float fontScale = 1.5f;
    ImGui::SetWindowFontScale(fontScale);
    const char* title = "GAME PAUSED";
    float textWidth = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - textWidth) * 0.5f);
    ImGui::Text("%s", title);
    ImGui::SetWindowFontScale(1.2f);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (showControls) {
        RenderControlsList();
    } else {
        float availY = ImGui::GetContentRegionAvail().y;
        float spacing = 10.0f;
        float btnHeight = (availY - spacing * 2.0f) / 3.0f;

        if (ImGui::Button("RESUME GAME", ImVec2(-1.0f, btnHeight))) {
            game.State = GAME_ACTIVE;
            game.SetMouseMode(true);
            showControls = false;
        }
        ImGui::Dummy(ImVec2(0.0f, spacing));

        if (ImGui::Button("CONTROLS", ImVec2(-1.0f, btnHeight))) {
            showControls = true;
        }
        ImGui::Dummy(ImVec2(0.0f, spacing));

        if (ImGui::Button("QUIT GAME", ImVec2(-1.0f, btnHeight))) {
            glfwSetWindowShouldClose(game.GetWindow(), true);
        }
    }
    ImGui::End();
}

void UIManager::RenderHUD(Game& game) {
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
    
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Press ESC to Pause");
    
    ImGui::End();
}