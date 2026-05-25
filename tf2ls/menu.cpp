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
bool espRedTeam = false;
bool espBlueTeam = false;

bool espBox, espFilledBox = false;
bool espHealth, espHealthText = false;

bool espSnaplines = false;
bool espClassName = false;

bool visualsParts = false;
float visualsParts_He = 1.f;
float visualsParts_To = 1.f;
float visualsParts_Ha = 1.f;

float espBoxThickness = 1.5f;
float espFontSize = 14.0f;

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

        auto draw = ImGui::GetBackgroundDrawList();

        if (espEnable) {
            if (espRedTeam) {
                for (const auto& ent : g_EntityList.TeamR) {
                    Vector3 headPos = ent.m_vecOrigin;
                    headPos.z += 72.0f;

                    Vector3 feetPos = ent.m_vecOrigin;

                    Vector2 screenHead = C_View::WorldToScreen(headPos);
                    Vector2 screenFeet = C_View::WorldToScreen(feetPos);

                    if (screenFeet.x <= 0 || screenFeet.y <= 0 ||
                        screenFeet.x > C_View::width || screenFeet.y > C_View::height)
                        continue;

                    float boxHeight = screenFeet.y - screenHead.y;
                    float boxWidth = boxHeight * 0.45f;

                    ImVec2 topLeft(screenHead.x - boxWidth / 2, screenHead.y);
                    ImVec2 bottomRight(screenHead.x + boxWidth / 2, screenFeet.y);
                    ImVec2 center(screenHead.x, screenHead.y);

                    ImColor color = ImColor(255, 0, 0, 255);

                    if (espBox) {
                        draw->AddRect(topLeft, bottomRight, color, 0.0f, 0, espBoxThickness);
                    }

                    if (espFilledBox) {
                        draw->AddRectFilled(topLeft, bottomRight, ImColor(255, 0, 0, 30));
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
                            ImColor(255, 0, 0, 100), 1.0f);
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
                }
            }

            if (espBlueTeam) {
                for (const auto& ent : g_EntityList.TeamB) {
                    Vector3 headPos = ent.m_vecOrigin;
                    headPos.z += 72.0f;

                    Vector3 feetPos = ent.m_vecOrigin;

                    Vector2 screenHead = C_View::WorldToScreen(headPos);
                    Vector2 screenFeet = C_View::WorldToScreen(feetPos);

                    if (screenFeet.x <= 0 || screenFeet.y <= 0 ||
                        screenFeet.x > C_View::width || screenFeet.y > C_View::height)
                        continue;

                    float boxHeight = screenFeet.y - screenHead.y;
                    float boxWidth = boxHeight * 0.45f;

                    ImVec2 topLeft(screenHead.x - boxWidth / 2, screenHead.y);
                    ImVec2 bottomRight(screenHead.x + boxWidth / 2, screenFeet.y);
                    ImVec2 center(screenHead.x, screenHead.y);

                    ImColor color = ImColor(0, 100, 255, 255);

                    if (espBox) {
                        draw->AddRect(topLeft, bottomRight, color, 0.0f, 0, espBoxThickness);
                    }

                    if (espFilledBox) {
                        draw->AddRectFilled(topLeft, bottomRight, ImColor(0, 100, 255, 30));
                        draw->AddRect(topLeft, bottomRight, color, 0.0f, 0, 1.0f);
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
                            ImColor(0, 100, 255, 100), 1.0f);
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
                }
            }
        }

        if (visualsParts) {
            for (const auto& ent : g_EntityList.TeamR) {
                *(bool*)(ent.p_Base + 0x11DD) = true;
                *(float*)(ent.p_Base + 0x39F0) = visualsParts_He;
                *(float*)(ent.p_Base + 0x39F4) = visualsParts_To;
                *(float*)(ent.p_Base + 0x39F8) = visualsParts_Ha;
            }

            for (const auto& ent : g_EntityList.TeamB) {
                *(bool*)(ent.p_Base + 0x11DD) = true;
                *(float*)(ent.p_Base + 0x39F0) = visualsParts_He;
                *(float*)(ent.p_Base + 0x39F4) = visualsParts_To;
                *(float*)(ent.p_Base + 0x39F8) = visualsParts_Ha;
            }
        }
        else {
            for (const auto& ent : g_EntityList.TeamR) {
                *(float*)(ent.p_Base + 0x39F0) = 1.f;
                *(float*)(ent.p_Base + 0x39F4) = 1.f;
                *(float*)(ent.p_Base + 0x39F8) = 1.f;
            }

            for (const auto& ent : g_EntityList.TeamB) {
                *(float*)(ent.p_Base + 0x39F0) = 1.f;
                *(float*)(ent.p_Base + 0x39F4) = 1.f;
                *(float*)(ent.p_Base + 0x39F8) = 1.f;
            }
        }

        if (show_menu) {
            ImGui::Begin("NiggaMoves", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Checkbox("ESP Enable", &espEnable);
            ImGui::SameLine();
            ImGui::Checkbox("Red", &espRedTeam);
            ImGui::SameLine();
            ImGui::Checkbox("Blue", &espBlueTeam);
            
            ImGui::Separator();

            ImGui::Checkbox("Box", &espBox);
            ImGui::SameLine();
            ImGui::Checkbox("Filled Box", &espFilledBox);

            ImGui::Checkbox("Health Bar", &espHealth);
            ImGui::SameLine();
            ImGui::Checkbox("Health Text", &espHealthText);

            ImGui::Checkbox("Snaplines", &espSnaplines);

            ImGui::Checkbox("Class Name", &espClassName);

            ImGui::SliderFloat("Box Thickness", &espBoxThickness, 0.5f, 3.0f);

            ImGui::Separator();
            ImGui::Text("Visuals");

            ImGui::Checkbox("Parts Size", &visualsParts);
            ImGui::SliderFloat("Head Scale", &visualsParts_He, 1.f, 50.f);
            ImGui::SliderFloat("Torso Scale", &visualsParts_To, 1.f, 50.f);
            ImGui::SliderFloat("Hand Scale", &visualsParts_Ha, 1.f, 50.f);

            ImGui::End();
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void InitMenu()
{
    Sleep(5000);
    MessageBoxA(0, "click OK once fully loaded", "load", MB_OK);

    client_dll = GetModuleHandleA("client.dll");
    engine_dll = GetModuleHandleA("engine.dll");

    if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
    {
        kiero::bind(17, (void**)&oPresent, hkPresent);
        kiero::bind(16, (void**)&oReset, hkReset);
    }
    return;
}