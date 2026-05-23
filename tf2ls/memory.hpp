#pragma once

#include "framework.h"

class memory {
public:
	static std::vector<int> PatternToInt(const char* szPattern);
	static uintptr_t FindSignature(const wchar_t* szModule, const char* szPattern);
};