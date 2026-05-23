#pragma once

#include "framework.h"
#include "memory.hpp"

namespace C_View {

	HWND hwnd;

	struct View_Matrix {
		float a11, a12, a13, a14;
		float a21, a22, a23, a24;
		float a31, a32, a33, a34;
		float a41, a42, a43, a44;
	};

	inline View_Matrix vm;

	inline float width, height;

	inline void Update() {
		if (!hwnd) 
			hwnd = FindWindowA("Valve001", NULL);

		RECT rect;
		GetClientRect(hwnd, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;

		float* p1 = (float*)((uintptr_t)engine_dll + 0x6A2C08 - 0x38); //pray for this magic offset nigga
		if (!p1) return;

		/* Later mb or do it yourself
		* 
		if (!IClientFov) {
			uintptr_t fovsigAddr = memory::FindSignature(L"client.dll",
				"49 8D 5C 24 48 F3 41 0F 11 44 24 68 45 33 C0 48 8B 05 ? ? ? ? BA AC 00 00 00");

			if (fovsigAddr) {
				uintptr_t movInstr = fovsigAddr + 14;
				uintptr_t nextInstr = movInstr + 7;
				int32_t ripOffset = *reinterpret_cast<int32_t*>(movInstr + 3);
				IClientFov = nextInstr + ripOffset;
			}
		}
		*/


		memcpy(&vm, p1, sizeof(vm));
	}

	inline Vector2 WorldToScreen(Vector3 world_pos) {
		Vector2 screen{ 0, 0 };

		float w = vm.a41 * world_pos.x + vm.a42 * world_pos.y + vm.a43 * world_pos.z + vm.a44;

		if (w < 0.001f) return screen;

		float x = vm.a11 * world_pos.x + vm.a12 * world_pos.y + vm.a13 * world_pos.z + vm.a14;
		float y = vm.a21 * world_pos.x + vm.a22 * world_pos.y + vm.a23 * world_pos.z + vm.a24;

		float camX = width / 2.0f;
		float camY = height / 2.0f;

		screen.x = camX + camX * x / w;
		screen.y = camY - camY * y / w;

		return screen;
	}
}