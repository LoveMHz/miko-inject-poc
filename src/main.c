#include <xboxkrnl/xboxkrnl.h>
#include <stdint.h>
#include "kernel/xboxkrnl_ext.h"
#include "detour.h"

PVOID __declspec(dllexport) ImportTableOffset = 0xAABBCCDD;

volatile uint32_t test = 0xDEADBABE;

typedef CDECL void _XboxMain(VOID);
_XboxMain *XboxMain = (_XboxMain*)0x000D7FA0;

typedef CDECL void _GameMain(VOID);
_GameMain *GameMain = (_GameMain*)0x000D7F20;

void CDECL GameMainHook();

uint8_t *sys_wrk   = (uint8_t *)0x0038b4e8;
uint8_t *title_wrk = (uint8_t *)0x001b82ac;
uint8_t *outgame_wrk = (uint8_t *)0x003914b8;
uint8_t *DAT_001B82BA = (uint8_t *)0x001b82ba;
uint8_t *ap_wrk = (uint8_t *)0x001b8e18;

uint32_t *DAT_003024a0 = (uint8_t *)0x003024a0;


__attribute__ ((naked)) void WinMainCRTStartup(void) {
	// Jump to original entry point
	// NOTE: noreturn
	_asm {
		// Patch and preserve cr0
		mov eax, cr0
		mov ecx, eax
		and eax, 0xFFFEFFFF
		mov cr0, eax

		// Patch call to main() to jmp to our MainHook()
		// PUSH MainHook
		MOV AL, 0x68
		MOV [0x000DAB45], AL

		// FIX!!!! This should be MainHook()
		MOV EAX, 0x0040102E
		MOV [0x000DAB46], EAX

		// RET
		MOV AL, 0xC3
		MOV [0x000DAB4A], AL

		// Restore cr0
		MOV cr0, ECX

		// JMP to original entry()
		PUSH 0x000DAB5D
		RET
	};
}

void CDECL XboxMainHook() {
	// HACK: Resolve kernel imports by having the build_xbe.py script inject the offset for
	//       IMAGE_IMPORT_DESCRIPTOR into the ImportTableOffset variable and then have the
	//       internal XepResolveImageImports() kernel function resolve the imports.

	// NOTE: This targets the 5838 kernel and is a bit cleaner than resolving imports manually.
	_XepResolveImageImports *XepResolveImageImports = (_XepResolveImageImports*)0x8002F192;
	XepResolveImageImports((PVOID)0x80010000, ImportTableOffset);

	if(test == 0)
		test = 2;

	test = 1;

	DbgPrint("[%s] Setting up hooks\n", __FUNCTION__);
	detour_call((char *)0x00011F78, (char *)GameMainHook);

	XboxMain();
}

void CDECL GameMainHook() {

	DbgPrint("[%s] sys_wrk: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
		 __FUNCTION__,
		sys_wrk[0], sys_wrk[1], sys_wrk[2], sys_wrk[3], sys_wrk[4], sys_wrk[5], sys_wrk[6], sys_wrk[7],
		sys_wrk[8], sys_wrk[9], sys_wrk[10], sys_wrk[11], sys_wrk[12], sys_wrk[13], sys_wrk[14], sys_wrk[15]
	);
	DbgPrint("[%s] sys_wrk[23]: %02X\n", __FUNCTION__, sys_wrk[23]);
	DbgPrint("[%s] title_wrk: %02X %02X %02X %02X %02X\n",__FUNCTION__,
		title_wrk[0], title_wrk[1], title_wrk[2], title_wrk[3], title_wrk[4]
	);
	DbgPrint("[%s] outgame_wrk: %02X %02X %02X %02X %02X\n",__FUNCTION__,
		outgame_wrk[0], outgame_wrk[1], outgame_wrk[2], outgame_wrk[3], outgame_wrk[4]
	);

	//*DAT_001B82BA = 1;
	/*
	if(title_wrk[4] == 5) {
		title_wrk[4] = 9;
	}
*/
	*ap_wrk = 0xFF;
	GameMain();
	*DAT_003024a0 = 0xFFFFF;
	return;
}
