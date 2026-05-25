#pragma once

#include "framework.h"

struct CBoneMatrix {
    float m[3][4];

    Vector3 GetOrigin() const {
        return Vector3(m[0][3], m[1][3], m[2][3]);
    }
};

class C_Entity {
public:
    uintptr_t p_Base = NULL;

    uint32_t m_iTeamNum{ 0 };
    uint32_t m_iHealth{ 0 };
    uint32_t m_iLifeState{ 0 };
    uint32_t m_iMaxHealth{ 0 };
    uint8_t m_iDormant{ 0 };

    int m_iClass = 0;
    char m_szClassName[32] = {};

    Vector3 m_vecOrigin{ 0 };

    static constexpr int MAX_BONES = 96;
    Vector3 m_BonePositions[MAX_BONES];

    bool m_bBonesUpdated = false;


    void Update() {
        if (p_Base == NULL) return;

        m_iTeamNum = *(uint32_t*)(p_Base + 0xEC);
        m_iHealth = *(uint32_t*)(p_Base + 0xE4);
        m_iLifeState = *(uint32_t*)(p_Base + 0xE0);
        m_iMaxHealth = *(uint32_t*)(p_Base + 0x1E08);
        m_iDormant = *(uint8_t*)(p_Base + 0x230);

        memset(m_szClassName, 0, sizeof(m_szClassName));

        __try {
            const char* className = (const char*)(p_Base + 0x1BB4);

            if (!IsBadReadPtr(className, 1)) {
                if (isalpha((unsigned char)className[0]) || className[0] != '\0') {
                    strncpy_s(m_szClassName, className, sizeof(m_szClassName) - 1);

                    if (strstr(m_szClassName, "scout")) m_iClass = 1;
                    else if (strstr(m_szClassName, "sniper")) m_iClass = 2;
                    else if (strstr(m_szClassName, "soldier")) m_iClass = 3;
                    else if (strstr(m_szClassName, "demoman")) m_iClass = 4;
                    else if (strstr(m_szClassName, "medic")) m_iClass = 5;
                    else if (strstr(m_szClassName, "heavy")) m_iClass = 6;
                    else if (strstr(m_szClassName, "pyro")) m_iClass = 7;
                    else if (strstr(m_szClassName, "spy")) m_iClass = 8;
                    else if (strstr(m_szClassName, "engineer")) m_iClass = 9;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            strcpy_s(m_szClassName, "Unknown");
        }

        m_vecOrigin = {
            *(float*)(p_Base + 0x338),
            *(float*)(p_Base + 0x33C),
            *(float*)(p_Base + 0x340)
        };

        UpdateBones();
    }

    void UpdateBones() {
        m_bBonesUpdated = false;
        if (!p_Base) return;

        uintptr_t pMatrices = *(uintptr_t*)(p_Base + 0xB80);
        if (!pMatrices) return;

        __try {
            for (int i = 0; i < MAX_BONES; i++) {
                float* pMatrix = (float*)(pMatrices + i * 0x30);
                m_BonePositions[i] = Vector3(pMatrix[3], pMatrix[7], pMatrix[11]);
            }
            m_bBonesUpdated = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            m_bBonesUpdated = false;
        }
    }

    Vector3 GetBonePosition(int boneIndex) const {
        if (boneIndex < 0 || boneIndex >= MAX_BONES || !m_bBonesUpdated)
            return m_vecOrigin;
        return m_BonePositions[boneIndex];
    }

    bool IsValid() const {
        return m_iTeamNum > 1 && m_iLifeState == 2;
    }

    bool IsDormant() const {
        return m_iDormant == 255;
    }
};