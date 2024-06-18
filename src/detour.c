#include <stdint.h>
#include <windows.h>

void detour_call(char* src, char* dst)
{
	uint32_t oldCR0;

	__asm {
		mov eax, cr0
		mov oldCR0, eax
		and eax, 0FFFEFFFFh
		mov cr0, eax
	}

	intptr_t relativeAddress = (intptr_t)(dst - (intptr_t)src) - 5;

	*src = (char)'\xE8';
	*(intptr_t*)((intptr_t)src + 1) = relativeAddress;

	_asm {
		mov eax, oldCR0
		mov cr0, eax
	}
}
