#include "Game/UIManager.h"
#include "Game/Game.h"

// 定义统一的菜单宽度，保证切换页面时窗口大小不跳变
static const float MENU_WIDTH = 700.0f;
static const float BUTTON_WIDTH = 300.0f;
static const float BUTTON_HEIGHT = 50.0f;

UIManager::UIManager() : showControls(false) {}

UIManager::~UIManager()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::Init(GLFWwindow *window)
{
    // 1. 创建上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    // 2. 设置风格
    ImGui::StyleColorsDark();

    // 设置圆角和边框风格
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding = 5.0f;

    // 3. 初始化绑定
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 放大字体以适应高分屏
    io.FontGlobalScale = 2.0f;
}

void UIManager::Render(Game &game)
{
    // 开始新的一帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 根据游戏状态分发渲染逻辑
    if (game.State == GAME_MENU)
    {
        RenderMainMenu(game);
    }
    else if (game.State == GAME_PAUSED)
    {
        RenderPauseMenu(game);
    }
    else if (game.State == GAME_ACTIVE)
    {
        RenderHUD(game);
    }

    // 渲染指令提交
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// === 辅助逻辑实现 ===

// 辅助函数：绘制居中的按钮
static bool CenteredButton(const char *label, float alignmentWidth)
{
    // 计算居中位置
    float windowWidth = ImGui::GetWindowSize().x;
    float textWidth = alignmentWidth;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    return ImGui::Button(label, ImVec2(textWidth, BUTTON_HEIGHT));
}

void UIManager::RenderControlsList()
{
    // 这里的 Dummy 不需要设得很大，因为外部容器已经被撑开了
    // 我们只需要处理好列的布局
    ImGui::Spacing();

    ImGui::Columns(2, "controls_columns", false); // false = 隐藏竖线

    // 设置第一列宽度，给按键描述留出足够空间
    ImGui::SetColumnWidth(0, 320.0f);

    // 辅助宏，简化代码
    auto DrawControl = [](const char *key, const char *desc)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", key);
        ImGui::NextColumn();
        ImGui::Text("%s", desc);
        ImGui::NextColumn();
        ImGui::Separator(); // 添加分隔线更清晰
    };

    DrawControl("W / A / S / D", "Move Character");
    DrawControl("Mouse", "Rotate Camera");
    DrawControl("TAB", "Switch View (1st/3rd)");
    DrawControl("T", "Switch Character");
    DrawControl("Mouse Left (Hold)", "Swing Sword");
    DrawControl("F", "Toggle Friend Follow");
    DrawControl("Q / E (Hold)", "Shake Head");
    DrawControl("SPACE", "Jump");
    DrawControl("B", "Toggle Day/Night");
    DrawControl("ESC", "Pause / Resume");

    ImGui::Columns(1); // 恢复单列模式

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    // 返回按钮也居中
    if (CenteredButton("BACK", BUTTON_WIDTH))
    {
        showControls = false;
    }
}

void UIManager::RenderMainMenu(Game &game)
{
    ImGuiIO &io = ImGui::GetIO();
    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    // 开启窗口
    ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    // 强制设置一个固定宽度的隐形横条
    // 无论显示按钮还是列表，窗口宽度都固定为 MENU_WIDTH (700)，彻底解决切换时的抖动
    ImGui::Dummy(ImVec2(MENU_WIDTH, 0.0f));

    // 标题居中
    float fontScale = 1.5f;
    ImGui::SetWindowFontScale(fontScale);
    const char *title = "Steve's Adventure";
    float textWidth = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - textWidth) * 0.5f);
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", title); // 给标题加个淡蓝色
    ImGui::SetWindowFontScale(1.2f);                                 // 恢复按钮字体大小

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    if (showControls)
    {
        RenderControlsList();
    }
    else
    {
        // 增加一些垂直间距，让布局不那么拥挤
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (CenteredButton("START GAME", BUTTON_WIDTH))
        {
            game.State = GAME_ACTIVE;
            game.SetMouseMode(true);
            showControls = false;
        }

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (CenteredButton("CONTROLS", BUTTON_WIDTH))
        {
            showControls = true;
        }

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (CenteredButton("QUIT", BUTTON_WIDTH))
        {
            glfwSetWindowShouldClose(game.GetWindow(), true);
        }
    }

    // 底部留白
    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    ImGui::End();
}

void UIManager::RenderPauseMenu(Game &game)
{
    ImGuiIO &io = ImGui::GetIO();
    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::Begin("Pause Menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    // 同样强制宽度
    ImGui::Dummy(ImVec2(MENU_WIDTH, 0.0f));

    // 标题
    float fontScale = 1.5f;
    ImGui::SetWindowFontScale(fontScale);
    const char *title = "GAME PAUSED";
    float textWidth = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - textWidth) * 0.5f);
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", title); // 红色标题
    ImGui::SetWindowFontScale(1.2f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    if (showControls)
    {
        RenderControlsList();
    }
    else
    {
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (CenteredButton("RESUME GAME", BUTTON_WIDTH))
        {
            game.State = GAME_ACTIVE;
            game.SetMouseMode(true);
            showControls = false;
        }

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (CenteredButton("CONTROLS", BUTTON_WIDTH))
        {
            showControls = true;
        }

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (CenteredButton("QUIT GAME", BUTTON_WIDTH))
        {
            glfwSetWindowShouldClose(game.GetWindow(), true);
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    ImGui::End();
}

void UIManager::RenderHUD(Game &game)
{
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.7f), "Press ESC to Pause");

    // 如果开启了跟随模式，显示一个提示
    if (game.isFollowing)
    { // 注意：需要确保 Game 类里的 isFollowing 是 public 的，或者有 getter
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ Follow Mode ON ]");
    }

    ImGui::End();
}