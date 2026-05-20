#include "framework.h"

#include "menu.hpp"

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved){
    if (DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE hM = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)InitMenu, hModule, 0, nullptr);
        CloseHandle(hM);
    }
    return TRUE;
}