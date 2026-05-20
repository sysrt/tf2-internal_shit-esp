#pragma once

#include "framework.h"
#include "C_TFPlayer.hpp"

#define RED 2
#define BLUE 3

class C_ClientEntityList {
public:
    uintptr_t p_Base{ 0 };

    std::vector<C_Entity> TeamB;
    std::vector<C_Entity> TeamR;

    void Update() {
        TeamB.clear();
        TeamR.clear();

        uintptr_t entityList = *(uintptr_t*)((uintptr_t)client_dll + 0xFDA328);
        if (entityList == NULL) return;

        uintptr_t worldCheck = *(uintptr_t*)(entityList + 0x08);
        if (worldCheck == NULL) return;

        for (int i = 1; i < 32; i++) {
            uintptr_t entityPtr = *(uintptr_t*)(entityList + 0x28 + (i * 0x20));

            if (entityPtr == NULL || entityPtr < 0x10000) continue;

            C_Entity ent;
            ent.p_Base = entityPtr;
            ent.Update();

            if (!ent.IsValid()) continue;

            if (ent.m_iTeamNum == RED) {
                TeamR.push_back(ent);
            }
            else if (ent.m_iTeamNum == BLUE) {
                TeamB.push_back(ent);
            }
        }
    }
};