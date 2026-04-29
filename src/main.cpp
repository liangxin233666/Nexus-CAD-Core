#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include <windows.h> 

#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3.h> 

#ifdef _WIN32
#include <GLFW/glfw3native.h> 
#endif

#include "resource.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "geometry/CurveProcessor.h"
#include "editor/CurveEditor.h"
#include <string>

// 根据深浅色模式配置 ImGui 的基础样式和颜色
void SetupModernImGuiStyle(bool isLightMode) {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.ItemSpacing = ImVec2(12, 10);
    style.FramePadding = ImVec2(10, 6);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;

    ImVec4* colors = style.Colors;
    if (isLightMode) {
        ImGui::StyleColorsLight();
        colors[ImGuiCol_WindowBg] = ImVec4(0.96f, 0.96f, 0.98f, 0.96f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.88f, 0.88f, 0.92f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.88f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.85f, 0.85f, 0.90f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.80f, 0.82f, 0.88f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.70f, 0.75f, 0.85f, 1.00f);
    }
    else {
        ImGui::StyleColorsDark();
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.96f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.28f, 0.35f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.35f, 0.45f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.85f, 0.80f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.50f, 0.95f, 0.90f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.30f, 0.75f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.75f, 1.00f, 1.00f);
    }
}

// 将曲率绝对值映射到 RGB 颜色，用于渲染热力图（从青色过渡到红色）
ImU32 GetHeatmapColor(double kappa, double maxKappa = 0.003) {
    double t = std::min(std::abs(kappa) / maxKappa, 1.0);
    int r = static_cast<int>(std::max(0.0, std::min(1.0, 2.0 * t - 0.5)) * 255);
    int g = static_cast<int>(std::max(0.0, std::min(1.0, 1.5 - 2.0 * t)) * 200);
    int b = static_cast<int>(std::max(0.0, std::min(1.0, 1.0 - t + 0.2)) * 255);
    return IM_COL32(r, g, b, 255);
}

void SetNativeWindowIcon(GLFWwindow* window) {
#ifdef _WIN32
    // 获取 GLFW 窗口背后的 Windows 原生句柄 (HWND)
    HWND hwnd = glfwGetWin32Window(window);

    // 获取当前程序的实例句柄
    HINSTANCE hInst = GetModuleHandle(NULL);

    // 从资源中加载图标 (IDI_APP_ICON 是在 .rc 里定义的)
    // 这里分别加载大图标（任务栏）和小图标（窗口左上角）
    HICON hIconBig = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    HICON hIconSmall = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    // 发送消息给 Windows 窗口，设置图标
    if (hIconBig) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
    }
    if (hIconSmall) {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
    }
#endif
}

int main() {
    // 1. 初始化 GLFW 和 OpenGL 环境
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8); // 启用 8x 多重采样抗锯齿

    GLFWwindow* window = glfwCreateWindow(1600, 1000, "Nexus CAD Core", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    SetNativeWindowIcon(window);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 开启垂直同步

    // 2. 初始化 ImGui 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 加载中文字体，如果找不到微软雅黑则降级使用黑体
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 20.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    if (!font) font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/simhei.ttf", 20.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // 3. 业务状态初始化
    CurveEditor editor;
    editor.init();
    bool isDragging = false;
    double currentSnapT = -1.0; // 记录当前鼠标投影在曲线上的参数位置

    // 4. 主渲染循环
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // 处理鼠标输入，仅在 ImGui 未占用鼠标时传递给编辑器
        if (!io.WantCaptureMouse) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                editor.onMouseDown(io.MousePos.x, io.MousePos.y, false);
                isDragging = true;
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                editor.onMouseDown(io.MousePos.x, io.MousePos.y, true);
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                editor.onMouseUp();
                isDragging = false;
            }
            if (isDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                editor.onMouseDrag(io.MousePos.x, io.MousePos.y);
            }
        }

        // 处理键盘输入（空格键切换连续性）
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            editor.toggleContinuityForSelection();
        }

        // 开始构建当前帧的 UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        SetupModernImGuiStyle(editor.isLightMode);

        // --- 面板 1：参数控制面板 ---
        ImGui::SetNextWindowPos(ImVec2(25, 25), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(420, 850), ImGuiCond_FirstUseEver);
        ImGui::Begin("参数控制面板", NULL, ImGuiWindowFlags_NoCollapse);

        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "系统设置");
        ImGui::Checkbox("启用浅色模式", &editor.isLightMode);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("基础显示", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("显示曲率热力图颜色", &editor.showHeatmapCurve);
            ImGui::Checkbox("启用鼠标对齐吸附", &editor.enableSnapping);
            ImGui::Checkbox("显示密切圆与法向标记", &editor.showOsculatingCircle);
            ImGui::Checkbox("显示外接凸包", &editor.showConvexHull);
        }

        if (ImGui::CollapsingHeader("几何分析", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("显示曲率梳", &editor.showCurvature);
            if (editor.showCurvature) ImGui::SliderFloat("曲率梳缩放", &editor.curvatureScale, 5000.0f, 150000.0f);

            ImGui::Checkbox("显示等距偏移曲线", &editor.showOffset);
            if (editor.showOffset) ImGui::SliderFloat("偏移距离", &editor.offsetDistance, -150.0f, 150.0f);

            ImGui::Checkbox("显示极值拐点", &editor.showInflections);
            ImGui::Checkbox("显示参数化速矢图", &editor.showHodograph);
        }

        if (ImGui::CollapsingHeader("拓扑运算", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("执行拉普拉斯平滑", ImVec2(-1, 45))) {
                editor.spline.fairCurve(5);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("对所有内部控制点执行平滑操作，减小曲线局部变形");

            if (ImGui::Button("在线段处执行细分", ImVec2(-1, 45))) {
                editor.subdivideAtSelected();
            }
        }

        ImGui::End();

        // --- 面板 2：实时数据面板 ---
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 380, 25), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(350, 180), ImGuiCond_Always);
        ImGui::Begin("数据遥测面板", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.7f, 1.0f), "曲线状态监控");
        ImGui::Separator();
        ImGui::Text("控制点总数: %zu", editor.spline.points.size());
        ImGui::Text("预估总长度: %.2f mm", editor.spline.calculateLength());
        ImGui::Text("格林公式面积: %.2f mm^2", editor.spline.calculateArea());
        ImGui::Separator();

        // 渲染当前鼠标吸附位置的局部计算数据
        if (currentSnapT >= 0.0) {
            double kap = editor.spline.getCurvature(currentSnapT);
            ImGui::Text("当前位置 T参数: %.4f", currentSnapT);
            ImGui::Text("局部曲率 (κ): %.6f", kap);
            ImGui::Text("曲率半径 (R): %.2f mm", (std::abs(kap) > 1e-6) ? (1.0 / std::abs(kap)) : 99999.9);
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "将鼠标悬停在曲线上查看详细数据...");
        }
        ImGui::End();

        // --- 开始几何图元渲染 ---
        ImDrawList* bgDraw = ImGui::GetBackgroundDrawList();

        // 提取主题颜色
        ImU32 colWireframe = editor.isLightMode ? IM_COL32(180, 180, 190, 180) : IM_COL32(80, 80, 95, 180);
        ImU32 colAnchor = editor.isLightMode ? IM_COL32(40, 40, 50, 255) : IM_COL32(240, 240, 250, 255);
        ImU32 colHandle = editor.isLightMode ? IM_COL32(255, 100, 0, 255) : IM_COL32(255, 180, 0, 255);
        ImU32 colCurveStd = editor.isLightMode ? IM_COL32(30, 30, 40, 255) : IM_COL32(0, 220, 255, 255);
        ImU32 colText = editor.isLightMode ? IM_COL32(80, 80, 90, 255) : IM_COL32(200, 200, 220, 255);

        // 1. 绘制凸包多边形
        if (editor.showConvexHull) {
            auto hull = CurveProcessor::computeConvexHull(editor.spline.points);
            if (hull.size() >= 3) {
                std::vector<ImVec2> imPts;
                imPts.reserve(hull.size());
                for (auto& p : hull) imPts.push_back(ImVec2(static_cast<float>(p.x), static_cast<float>(p.y)));
                bgDraw->AddConvexPolyFilled(imPts.data(), static_cast<int>(imPts.size()), editor.isLightMode ? IM_COL32(0, 0, 0, 8) : IM_COL32(255, 255, 255, 8));
                bgDraw->AddPolyline(imPts.data(), static_cast<int>(imPts.size()), colWireframe, ImDrawFlags_Closed, 1.5f);
            }
        }

        // 2. 绘制主曲线（根据是否开启热力图选择颜色）
        auto curvePts = CurveProcessor::discretizeCurveWithCurvature(editor.spline, 150);
        for (size_t i = 0; i < curvePts.size() - 1; ++i) {
            ImU32 lineColor = editor.showHeatmapCurve ? GetHeatmapColor((curvePts[i].kappa + curvePts[i + 1].kappa) / 2.0) : colCurveStd;
            bgDraw->AddLine(ImVec2(static_cast<float>(curvePts[i].pos.x), static_cast<float>(curvePts[i].pos.y)),
                ImVec2(static_cast<float>(curvePts[i + 1].pos.x), static_cast<float>(curvePts[i + 1].pos.y)), lineColor, 4.5f);
        }

        // 3. 绘制曲率梳与偏移线
        if (editor.showCurvature) {
            auto combs = CurveProcessor::generateCurvatureComb(editor.spline, editor.curvatureScale, 150);
            for (auto& line : combs)
                bgDraw->AddLine(ImVec2(static_cast<float>(line.start.x), static_cast<float>(line.start.y)),
                    ImVec2(static_cast<float>(line.end.x), static_cast<float>(line.end.y)), IM_COL32(50, 220, 100, 150), 1.0f);
        }
        if (editor.showOffset) {
            auto offsetPts = CurveProcessor::generateOffsetCurve(editor.spline, editor.offsetDistance, 150);
            for (size_t i = 0; i < offsetPts.size() - 1; ++i)
                bgDraw->AddLine(ImVec2(static_cast<float>(offsetPts[i].x), static_cast<float>(offsetPts[i].y)),
                    ImVec2(static_cast<float>(offsetPts[i + 1].x), static_cast<float>(offsetPts[i + 1].y)), IM_COL32(255, 100, 150, 180), 2.5f);
        }

        // 4. 绘制独立显示在右下角的速矢图（一阶导数图）
        if (editor.showHodograph) {
            Vec2 origin(io.DisplaySize.x - 200, io.DisplaySize.y - 200);
            bgDraw->AddCircleFilled(ImVec2(static_cast<float>(origin.x), static_cast<float>(origin.y)), 5.0f, IM_COL32(150, 150, 150, 255));
            bgDraw->AddText(ImVec2(static_cast<float>(origin.x - 40), static_cast<float>(origin.y + 15)), colText, "速矢图原点");
            auto hodoPts = CurveProcessor::generateHodograph(editor.spline, origin, 0.2, 120);
            for (size_t i = 0; i < hodoPts.size() - 1; ++i) {
                bgDraw->AddLine(ImVec2(static_cast<float>(hodoPts[i].x), static_cast<float>(hodoPts[i].y)),
                    ImVec2(static_cast<float>(hodoPts[i + 1].x), static_cast<float>(hodoPts[i + 1].y)), IM_COL32(180, 100, 255, 200), 2.0f);
            }
        }

        // 5. 绘制拐点标记
        if (editor.showInflections) {
            auto inflections = CurveProcessor::findInflectionPoints(editor.spline);
            for (auto& p : inflections) {
                float px = static_cast<float>(p.x), py = static_cast<float>(p.y);
                bgDraw->AddQuadFilled(ImVec2(px, py - 8), ImVec2(px + 8, py), ImVec2(px, py + 8), ImVec2(px - 8, py), IM_COL32(255, 50, 50, 255));
                bgDraw->AddText(ImVec2(px + 12, py - 12), IM_COL32(255, 50, 50, 255), "拐点");
            }
        }

        // 6. 绘制控制点和拓扑连线
        for (size_t i = 0; i < editor.spline.points.size(); ++i) {
            auto pt = editor.spline.points[i];
            float px = static_cast<float>(pt.x), py = static_cast<float>(pt.y);
            bool isSelected = (static_cast<int>(i) == editor.selectedPointIndex);

            if (i % 3 != 0) { // 手柄点
                size_t anchorIdx = (i + 1) % 3 == 0 ? i + 1 : i - 1;
                auto anchorPt = editor.spline.points[anchorIdx];
                bgDraw->AddLine(ImVec2(px, py), ImVec2(static_cast<float>(anchorPt.x), static_cast<float>(anchorPt.y)), colWireframe, 1.5f);
                bgDraw->AddCircleFilled(ImVec2(px, py), isSelected ? 6.5f : 4.5f, isSelected ? IM_COL32(255, 50, 50, 255) : colHandle);
            }
            else { // 锚点
                float size = isSelected ? 8.5f : 6.0f;
                bgDraw->AddRectFilled(ImVec2(px - size, py - size), ImVec2(px + size, py + size), isSelected ? IM_COL32(255, 50, 50, 255) : colAnchor);

                // 显示该锚点的连续性模式标签
                ContinuityMode mode = editor.spline.modes[i / 3];
                const char* modeText = (mode == ContinuityMode::Free) ? "C0" : (mode == ContinuityMode::Aligned ? "C1" : "C2");
                bgDraw->AddText(ImVec2(px + 10, py - 20), colText, modeText);
            }
        }

        // 7. 处理鼠标靠近曲线时的吸附投影与密切圆渲染
        currentSnapT = -1.0;
        if (editor.enableSnapping && !io.WantCaptureMouse && editor.draggingPointIndex == -1) {
            Vec2 m(io.MousePos.x, io.MousePos.y);
            double t = CurveProcessor::getClosestPointT(editor.spline, m);
            Vec2 closest = editor.spline.getPoint(t);

            // 判断鼠标距离是否进入吸附范围
            if (m.distanceTo(closest) < 120.0) {
                currentSnapT = t;
                float cx = static_cast<float>(closest.x), cy = static_cast<float>(closest.y);

                // 绘制从鼠标到吸附点的连接线
                bgDraw->AddLine(ImVec2(static_cast<float>(m.x), static_cast<float>(m.y)), ImVec2(cx, cy), IM_COL32(100, 200, 255, 180), 1.5f);
                bgDraw->AddCircleFilled(ImVec2(cx, cy), 5.5f, IM_COL32(50, 200, 255, 255));

                if (editor.showOsculatingCircle) {
                    double kappa = editor.spline.getCurvature(t);
                    if (std::abs(kappa) > 1e-6) {
                        double radius = 1.0 / std::abs(kappa);

                        // 计算切线和法线方向（Frenet标架）
                        Vec2 tangent = editor.spline.getDerivative(t).normalize();
                        Vec2 normal = tangent.perpendicular();
                        if (kappa < 0) normal = normal * -1.0; // 根据曲率符号翻转法线朝向圆心

                        // 限制可视化的圆圈最大半径，防止平直区间导致的渲染异常
                        double renderRadius = std::min(radius, 5000.0);
                        Vec2 circleCenter = closest + normal * renderRadius;

                        // 绘制切线（红色）和法线（绿色）
                        bgDraw->AddLine(ImVec2(cx, cy), ImVec2(cx + static_cast<float>(tangent.x * 60), cy + static_cast<float>(tangent.y * 60)), IM_COL32(255, 80, 80, 255), 2.5f);
                        bgDraw->AddLine(ImVec2(cx, cy), ImVec2(cx + static_cast<float>(normal.x * 60), cy + static_cast<float>(normal.y * 60)), IM_COL32(80, 255, 80, 255), 2.5f);

                        // 绘制代表曲率的密切圆
                        bgDraw->AddCircle(ImVec2(static_cast<float>(circleCenter.x), static_cast<float>(circleCenter.y)), static_cast<float>(renderRadius), IM_COL32(255, 200, 0, 100), 120, 1.5f);

                        std::string lbl = "R= " + std::to_string(static_cast<int>(radius));
                        bgDraw->AddText(ImVec2(cx + 15, cy + 15), IM_COL32(255, 200, 0, 255), lbl.c_str());
                    }
                }
                else {
                    bgDraw->AddText(ImVec2(cx + 8, cy + 8), IM_COL32(100, 200, 255, 255), "吸附");
                }
            }
        }

        // 渲染并交换缓冲区
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        // 根据浅色/深色模式设定清除背景色
        if (editor.isLightMode) glClearColor(0.92f, 0.92f, 0.95f, 1.0f);
        else glClearColor(0.06f, 0.06f, 0.08f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // 资源释放
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}