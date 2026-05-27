#include "framework.h"

#include <d3d9.h>
#include "kiero.h"

#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_dx9.h"
#include "ext/imgui/imgui_impl_win32.h"

#include "menu.hpp"

#include "C_View.hpp"
#include "C_TFPlayer.hpp"
#include "C_ClientEntityList.hpp"

struct SkeletonMap {
    int head;
    int neck;
    int shoulder_L;
    int shoulder_R;
    int elbow_L;
    int elbow_R;
    int palm_L;
    int palm_R;
    int root;
    int hip_L;
    int hip_R;
    int knee_L;
    int knee_R;
    int foot_L;
    int foot_R;
};

SkeletonMap g_BoneMaps[10] = {
    {0},

    {6,5,9,10,11,12,19,20,0,13,14,15,16,17,18}, // 1 scout

    {6,5,9,10,11,12,19,20,0,13,14,15,16,17,18}, // 2 sniper

    {32,6,9,10,11,12,19,20,0,13,14,15,16,17,18}, // 3 soldier

    {16,15,6,8,19,14,17,20,1,9,11,10,12,25,26}, // 4 demoman

    {6,5,9,10,11,12,17,18,0,13,14,15,16,34,35}, // 5 medic

    {6,5,9,10,11,12,17,18,0,13,14,15,16,29,30}, // 6 heavy

    {6,5,8,12,9,13,10,14,0,15,19,16,20,17,21}, // 7 pyro

    {6,5,10,11,12,13,14,15,0,16,17,18,19,20,21}, // 8 spy

    {61,8,12,15,13,16,20,19,0,9,1,10,2,17,18}  // 9 engineer
};

typedef long(__stdcall* Reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
Reset oReset = NULL;

typedef long(__stdcall* Present)(LPDIRECT3DDEVICE9, const RECT*, const RECT*, HWND, const RGNDATA*);
Present oPresent = NULL;

WNDPROC oWndProc;
HWND window = NULL;

bool init = false;
bool show_menu = true;

C_ClientEntityList g_EntityList;

bool espEnable = false;
bool espInterpolate = false;

bool espEnemy = false;
bool espTeam = false;

bool espBox, espFilledBox = false;
bool espHealth, espHealthText = false;

bool espSnaplines = false;
bool espClassName = false;

bool espSkeleton = false;

bool visualsParts = false;
Vector3 fvisualsParts = Vector3(1.f, 1.f, 1.f);

float espBoxThickness = 1.5f;
float espFontSize = 14.0f;

void SetScale(C_Entity ent, Vector3 scale) {
    *(float*)(ent.p_Base + 0x39F0) = scale.x;
    *(float*)(ent.p_Base + 0x39F4) = scale.y;
    *(float*)(ent.p_Base + 0x39F8) = scale.z;
}

void DrawSkeleton(C_Entity& ent, ImColor color, Vector2 screenHead, float boxHeight) {
    if (!ent.m_bBonesUpdated) return;
    if (ent.m_iClass < 1 || ent.m_iClass > 9) return;

    auto& map = g_BoneMaps[ent.m_iClass];
    auto draw = ImGui::GetBackgroundDrawList();

    int* b = (int*)&map;

    struct Conn { int from; int to; };
    Conn connections[] = {
        {0, 1},   // head → neck
        {1, 8},   // neck → root
        {1, 2},   // neck → shoulder_L
        {1, 3},   // neck → shoulder_R
        {2, 4},   // shoulder_L → elbow_L
        {3, 5},   // shoulder_R → elbow_R
        {4, 6},   // elbow_L → palm_L
        {5, 7},   // elbow_R → palm_R
        {8, 9},   // root → hip_L
        {8, 10},  // root → hip_R
        {9, 11},  // hip_L → knee_L
        {10, 12}, // hip_R → knee_R
        {11, 13}, // knee_L → foot_L
        {12, 14} // knee_R → foot_R
    };

    float circleRadius = boxHeight * 0.15f;
    if (circleRadius > 4.0f)  circleRadius = 4.0f;

    draw->AddCircle(ImVec2(screenHead.x, screenHead.y), circleRadius, color, 12, 0.1f);

    for (const auto& conn : connections) {
        Vector3 from = ent.GetBonePosition(b[conn.from]);
        Vector3 to = ent.GetBonePosition(b[conn.to]);

        Vector2 sFrom = C_View::WorldToScreen(from);
        Vector2 sTo = C_View::WorldToScreen(to);

        if (sFrom.x <= 0 || sFrom.y <= 0 || sTo.x <= 0 || sTo.y <= 0) continue;
        if (sFrom.x > C_View::width || sFrom.y > C_View::height) continue;
        if (sTo.x > C_View::width || sTo.y > C_View::height) continue;

        draw->AddLine(ImVec2(sFrom.x, sFrom.y), ImVec2(sTo.x, sTo.y), color, 0.1f);
    }
}

void DrawEntity(C_Entity ent) {
    auto draw = ImGui::GetBackgroundDrawList();

    auto& map = g_BoneMaps[ent.m_iClass];
    int* b = (int*)&map;

    bool isSpy = (ent.m_iClass == 8);
    
    Vector3 headPos, feetPos;

    if (!espInterpolate) {
        Vector3 rootPos = ent.GetBonePosition(b[8]);
        headPos = isSpy ? ent.m_vecOrigin : rootPos;
        headPos.z += isSpy ? 72.0f : 57.0f;

        feetPos = isSpy ? ent.m_vecOrigin : rootPos;
        feetPos.z -= isSpy ? 0.0f : 58.0f;
    }
    else {
        headPos = ent.m_vecOriginInterpolated;
        headPos.z += 72.0f;

        feetPos = ent.m_vecOriginInterpolated;
    }

    Vector2 screenHead = C_View::WorldToScreen(headPos);
    Vector2 screenFeet = C_View::WorldToScreen(feetPos);


    if (screenFeet.x <= 0 || screenFeet.y <= 0 ||
        screenFeet.x > C_View::width || screenFeet.y > C_View::height)
        return;

    float boxHeight = screenFeet.y - screenHead.y;
    float boxWidth = boxHeight * 0.45f;

    ImVec2 topLeft(screenHead.x - boxWidth / 2, screenHead.y);
    ImVec2 bottomRight(screenHead.x + boxWidth / 2, screenFeet.y);
    ImVec2 center(screenHead.x, screenHead.y);

    ImColor color = ImColor(ent.m_iTeamNum == RED_TEAM ? 255 : 0, 0, ent.m_iTeamNum == BLUE_TEAM ? 255 : 0, 255);
    ImColor colorBox = ImColor(ent.m_iTeamNum == RED_TEAM ? 255 : 0, 0, ent.m_iTeamNum == BLUE_TEAM ? 255 : 0, 30);
    ImColor colorSnap = ImColor(ent.m_iTeamNum == RED_TEAM ? 255 : 0, 0, ent.m_iTeamNum == BLUE_TEAM ? 255 : 0, 100);
    
    if (espBox) {
        draw->AddRect(topLeft, bottomRight, colorBox, 0.0f, 0, espBoxThickness * 0.5f);
    }

    if (espFilledBox) {
        draw->AddRectFilled(topLeft, bottomRight, colorBox);
        draw->AddRect(topLeft, bottomRight, color, 0.0f, 0, 1.0f);
    }

    if (espBox && !espFilledBox) {
        float cornerSize = boxWidth * 0.25f;
        draw->AddLine(ImVec2(topLeft.x, topLeft.y + cornerSize),
            ImVec2(topLeft.x, topLeft.y), color, 2.0f);
        draw->AddLine(ImVec2(topLeft.x, topLeft.y),
            ImVec2(topLeft.x + cornerSize, topLeft.y), color, 2.0f);
        draw->AddLine(ImVec2(bottomRight.x - cornerSize, topLeft.y),
            ImVec2(bottomRight.x, topLeft.y), color, 2.0f);
        draw->AddLine(ImVec2(bottomRight.x, topLeft.y),
            ImVec2(bottomRight.x, topLeft.y + cornerSize), color, 2.0f);
        draw->AddLine(ImVec2(topLeft.x, bottomRight.y - cornerSize),
            ImVec2(topLeft.x, bottomRight.y), color, 2.0f);
        draw->AddLine(ImVec2(topLeft.x, bottomRight.y),
            ImVec2(topLeft.x + cornerSize, bottomRight.y), color, 2.0f);
        draw->AddLine(ImVec2(bottomRight.x - cornerSize, bottomRight.y),
            ImVec2(bottomRight.x, bottomRight.y), color, 2.0f);
        draw->AddLine(ImVec2(bottomRight.x, bottomRight.y - cornerSize),
            ImVec2(bottomRight.x, bottomRight.y), color, 2.0f);
    }

    if (espHealth) {
        float barWidth = 4.0f;
        float barHeight = boxHeight;
        ImVec2 barTopLeft(topLeft.x - barWidth - 4, screenHead.y);
        ImVec2 barBottomRight(topLeft.x - 4, screenFeet.y);

        draw->AddRectFilled(barTopLeft, barBottomRight, ImColor(0, 0, 0, 180));

        float healthPercent = ent.m_iHealth / (float)ent.m_iMaxHealth;
        if (healthPercent > 1.0f) healthPercent = 1.0f;

        float healthHeight = barHeight * healthPercent;
        ImVec2 healthBarTop(barTopLeft.x, barBottomRight.y - healthHeight);

        ImColor healthColor;
        if (ent.m_iHealth > ent.m_iMaxHealth)
            healthColor = ImColor(0, 0, 255, 255);
        else if (healthPercent > 0.5f)
            healthColor = ImColor(0, 255, 0, 255);
        else if (healthPercent > 0.25f)
            healthColor = ImColor(255, 255, 0, 255);
        else
            healthColor = ImColor(255, 0, 0, 255);

        draw->AddRectFilled(healthBarTop, barBottomRight, healthColor);
        draw->AddRect(barTopLeft, barBottomRight, ImColor(255, 255, 255, 100), 0.0f, 0, 1.0f);
    }

    if (espSnaplines) {
        ImVec2 screenCenter(C_View::width / 2, C_View::height);
        draw->AddLine(screenCenter, ImVec2(center.x, screenFeet.y),
            colorSnap, 1.0f);
    }

    if (espHealthText) {
        char healthText[16];
        sprintf_s(healthText, "%dHP", ent.m_iHealth);
        draw->AddText(ImVec2(center.x - 15, screenFeet.y + 5),
            ImColor(255, 255, 255, 255), healthText);
    }

    if (espClassName) {
        char displayName[32];
        strcpy_s(displayName, ent.m_szClassName);
        if (displayName[0] >= 'a' && displayName[0] <= 'z') {
            displayName[0] = displayName[0] - 'a' + 'A';
        }

        draw->AddText(
            ImVec2(center.x - 15, screenHead.y - 20),
            ImColor(255, 255, 255, 255),
            displayName
        );
    }

    if (espSkeleton) {
        DrawSkeleton(ent, ImColor(255, 255, 255, 255), C_View::WorldToScreen(ent.GetBonePosition(b[0])), boxHeight);
    }
}

inline ImColor Vec4ToColor(const ImVec4& v) {
    return ImColor(v.x, v.y, v.z, v.w);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (show_menu) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        return true;
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

long __stdcall hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    long res = oReset(pDevice, pPresentationParameters);
    ImGui_ImplDX9_CreateDeviceObjects();
    return res;
}

long __stdcall hkPresent(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
    if (!init)
    {
        D3DDEVICE_CREATION_PARAMETERS params;
        if (SUCCEEDED(pDevice->GetCreationParameters(&params)))
        {
            window = params.hFocusWindow;
            if (window == NULL) window = GetForegroundWindow();

            ImGui::CreateContext();
            ApplyStyle();

            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX9_Init(pDevice);

            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

            init = true;
        }
    }

    if (GetAsyncKeyState(VK_INSERT) & 1) {
        show_menu = !show_menu;
    }

    if (init)
    {
        g_EntityList.Update();

        C_View::Update();

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (espEnable) {
            uint32_t nlocalTeam = 0;
            int localTeam = g_EntityList.GetLocalTeam();

            if (espEnemy || espTeam) {
                if (espEnemy) {
                    if (localTeam == RED_TEAM)
                        for (const auto& ent : g_EntityList.TeamB) DrawEntity(ent);
                    else if (localTeam == BLUE_TEAM)
                        for (const auto& ent : g_EntityList.TeamR) DrawEntity(ent);
                }
                if (espTeam) {
                    if (localTeam == RED_TEAM)
                        for (const auto& ent : g_EntityList.TeamR) DrawEntity(ent);
                    else if (localTeam == BLUE_TEAM)
                        for (const auto& ent : g_EntityList.TeamB) DrawEntity(ent);
                }
            }
        }

        if (visualsParts) {
            for (const auto& ent : g_EntityList.TeamR) {
                SetScale(ent, fvisualsParts);
            }

            for (const auto& ent : g_EntityList.TeamB) {
                SetScale(ent, fvisualsParts);
            }
        }
        else {
            for (const auto& ent : g_EntityList.TeamR) {
                SetScale(ent, Vector3(1.f, 1.f, 1.f));
            }

            for (const auto& ent : g_EntityList.TeamB) {
                SetScale(ent, Vector3(1.f, 1.f, 1.f));
            }
        }

        if (show_menu) {
            ImGui::Begin("NiggaMoves", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
            
            ImGui::Checkbox("ESP Enable", &espEnable);

            if (espEnable) {
                ImGui::Checkbox("Interpolated positions", &espInterpolate);
                ImGui::TextDisabled("Enable if boxes jittering");
            }

            ImGui::Separator();

            ImGui::Checkbox("Enemy", &espEnemy);
            ImGui::SameLine();
            ImGui::Checkbox("Team", &espTeam);

            ImGui::Checkbox("Box", &espBox);
            ImGui::SameLine();
            ImGui::Checkbox("Filled Box", &espFilledBox);

            ImGui::Checkbox("Health Bar", &espHealth);
            ImGui::SameLine();
            ImGui::Checkbox("Health Text", &espHealthText);

            ImGui::Checkbox("Snaplines", &espSnaplines);

            ImGui::Checkbox("Class Name", &espClassName);

            ImGui::Checkbox("Skeleton", &espSkeleton);

            ImGui::SliderFloat("Box Thickness", &espBoxThickness, 0.5f, 3.0f);

            ImGui::Separator();
            ImGui::Text("Visuals");

            ImGui::Checkbox("Parts Size", &visualsParts);
            ImGui::SliderFloat("Head Scale", &fvisualsParts.x, 1.f, 50.f);
            ImGui::SliderFloat("Torso Scale", &fvisualsParts.y, 1.f, 50.f);
            ImGui::SliderFloat("Hand Scale", &fvisualsParts.z, 1.f, 50.f);

            ImGui::Separator();

            ImGui::End();
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.94f);
    s.Colors[ImGuiCol_TitleBg] = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
    s.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    s.Colors[ImGuiCol_FrameBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    s.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    s.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    s.Colors[ImGuiCol_Button] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    s.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    s.Colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    s.Colors[ImGuiCol_Header] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    s.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    s.Colors[ImGuiCol_HeaderActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    s.Colors[ImGuiCol_CheckMark] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    s.Colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    s.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    s.Colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    s.Colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    s.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    s.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    s.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

    s.WindowRounding = 6.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 3.0f;
    s.ChildRounding = 4.0f;
    s.PopupRounding = 3.0f;
    s.ScrollbarRounding = 8.0f;

    s.WindowPadding = ImVec2(10.0f, 10.0f);
    s.FramePadding = ImVec2(6.0f, 4.0f);
    s.ItemSpacing = ImVec2(8.0f, 6.0f);
    s.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
}

void InitMenu()
{
    Sleep(5000);
    MessageBoxA(0, "Son 😭 Folk 😭 Sonkey 😭", "load", MB_OK);

    client_dll = GetModuleHandleA("client.dll");
    engine_dll = GetModuleHandleA("engine.dll");

    if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
    {
        kiero::bind(17, (void**)&oPresent, hkPresent);
        kiero::bind(16, (void**)&oReset, hkReset);
    }
    return;
}
