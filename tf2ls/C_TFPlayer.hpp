#pragma once

#include "framework.h"

class C_Entity {
public:
    uintptr_t p_Base = NULL;

    uint32_t m_iTeamNum{0};
    uint32_t m_iHealth{0};
    uint32_t m_iLifeState{ 0 };
    uint32_t m_iMaxHealth{ 0 };

    int m_iClass = 0;
    char m_szClassName[32] = {};

    //bool m_bIsABot{0};

    Vector3 m_vecOrigin;

    void Update() {
        if (p_Base == NULL) return;

        m_iTeamNum = *(uint32_t*)(p_Base + 0xEC);
        m_iHealth = *(uint32_t*)(p_Base + 0xE4);
        m_iLifeState = *(uint32_t*)(p_Base + 0xE0);
        m_iMaxHealth = *(uint32_t*)(p_Base + 0x1E08);

        //m_bIsABot = *(bool*)(p_Base + 0x23EF);

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
    }
    bool IsValid() const {
        return m_iTeamNum > 1 && m_iLifeState == 2;
    }
};