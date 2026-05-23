#pragma once

#include "framework.h"
#include "C_TFPlayer.hpp"
#include "memory.hpp"

#define RED 2
#define BLUE 3

class C_ClientEntityList {
private:
    uintptr_t m_entityList = 0;
    bool m_bInitialized = false;

    void Initialize() {
        uintptr_t sigAddr = memory::FindSignature(L"client.dll",
            "74 03 0F B7 C2 8B C8 48 8B 05 ? ? ? ? 48 83 C0 08");

        if (sigAddr) {
            uintptr_t movInstr = sigAddr + 7;
            int32_t ripOffset = *reinterpret_cast<int32_t*>(sigAddr + 10);
            uintptr_t targetPtr = movInstr + 7 + ripOffset;
            m_entityList = *reinterpret_cast<uintptr_t*>(targetPtr);
        }

        m_bInitialized = (m_entityList != 00);
    }

public:
    std::vector<C_Entity> TeamB;
    std::vector<C_Entity> TeamR;

    void Update() {
        if (!m_bInitialized) {
            Initialize();
            if (!m_bInitialized) return;
        }

        TeamB.clear();
        TeamR.clear();
        uintptr_t entityList = *reinterpret_cast<uintptr_t*>(&m_entityList);

        if (!entityList) return;

        TeamB.reserve(32);
        TeamR.reserve(32);

        for (int i = 0; i < 32; i++) {
            uintptr_t entityPtr = *reinterpret_cast<uintptr_t*>(entityList + 0x28 + (i * 0x20));

            if (!entityPtr || entityPtr < 0x10000) continue;

            C_Entity ent;
            ent.p_Base = entityPtr;
            ent.Update();

            if (!ent.IsValid()) continue;

            if (ent.m_iTeamNum == RED)
                TeamR.push_back(ent);
            else if (ent.m_iTeamNum == BLUE)
                TeamB.push_back(ent);
        }
    }
};