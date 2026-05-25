#pragma once

#include "framework.h"
#include "C_TFPlayer.hpp"
#include "memory.hpp"

#define RED 2
#define BLUE 3

class C_ClientEntityList {
private:
    uintptr_t m_entityList = 0;
    uintptr_t m_localplayer = 0;

    bool m_bInitialized = false;

    void Initialize() {
        uintptr_t entitylistsig = memory::FindSignature(L"client.dll",
            "74 03 0F B7 C2 8B C8 48 8B 05 ? ? ? ? 48 83 C0 08");

        uintptr_t localplayersig = memory::FindSignature(L"client.dll",
            "CC 40 53 48 83 EC 20 48 39 0D ? ? ? ? 48 8B D9");

        if (entitylistsig) {
            uintptr_t movInstr = entitylistsig + 7;
            int32_t ripOffset = *reinterpret_cast<int32_t*>(entitylistsig + 10);
            uintptr_t targetPtr = movInstr + 7 + ripOffset;
            m_entityList = *reinterpret_cast<uintptr_t*>(targetPtr);
        }

        if (localplayersig) {

            uintptr_t cmpInstr = localplayersig + 7;

            uintptr_t nextInstr = cmpInstr + 7;

            int32_t ripOffset = *reinterpret_cast<int32_t*>(cmpInstr + 3);

            m_localplayer = nextInstr + ripOffset;
        }

        m_bInitialized = (m_entityList != 00 && m_localplayer != 00);
    }

    uintptr_t GetLocalPlayer() {
        if (!m_localplayer) return 0;
        return *reinterpret_cast<uintptr_t*>(m_localplayer);
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
        uintptr_t localPlayer = GetLocalPlayer();

        if (!entityList || !localPlayer) return;

        TeamB.reserve(32);
        TeamR.reserve(32);

        for (int i = 0; i < 32; i++) {
            uintptr_t entityPtr = *reinterpret_cast<uintptr_t*>(entityList + 0x28 + (i * 0x20));

            if (!entityPtr || entityPtr < 0x10000) continue;

            if (entityPtr == localPlayer) continue;

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